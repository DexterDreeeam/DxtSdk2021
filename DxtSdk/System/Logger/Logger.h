#pragma once

template<typename ...Args>
_INLINE_ void log(Args...args);

template<typename ...Args>
_INLINE_ void err(Args...args);

#if DEBUG_LEVEL > DEBUG_LEVEL_CALIBRATION_LOG_NONE

namespace LoggerNs
{

    const s64 worker_count = 16LL;
    const s64 log_args_max_count = 16LL;
    const s64 log_line_max_length = 256LL;
    const s64 log_path_max_length = 512LL;

    class log_system_worker
    {
    public:
        log_system_worker() :
            log_file((outf)0),
            busy(1)
        {}

        void init(const char* base_folder, s64 mynumber)
        {
            assert(busy);
            memory_copy(base_folder, log_file_path, log_path_max_length);
            s64 len = str_len(log_file_path);
            log_file_path[len++] = 'l';
            log_file_path[len++] = 'o';
            log_file_path[len++] = 'g';
            log_file_path[len++] = '_';
            len += s64_to_text(mynumber, log_file_path + len);
            log_file_path[len++] = '.';
            log_file_path[len++] = 't';
            log_file_path[len++] = 'x';
            log_file_path[len++] = 't';
            log_file_path[len] = 0;
            log_file = output_file_create(log_file_path);
            assert(log_file);
            busy = 0;
        }

        void uninit()
        {
            s32 cnt = 0;
            while (busy && cnt < s32_max)
            {
                ++cnt;
                yield();
            }
            assert(cnt != s32_max);
            output_file_destroy(log_file);
            log_file = 0;
        }

        const char* mypath() const
        {
            return log_file_path;
        }

        boole try_get_control()
        {
            if (atom_exchange(busy, 1LL) == 0)
            {
                return boole::True;
            }
            else
            {
                return boole::False;
            }
        }

        void put_control()
        {
            assert(busy);
            busy = 0;
        }

        void write_log(const char* text, s64 write_len, u64 order)
        {
            char  buf[64];
            char* buf_ptr = buf;
            u64   thread_id = thrd_myid();
            date  mydate = date_query();

            *buf_ptr++ = '[';
            buf_ptr += u64_to_text_hex_padding(thread_id, buf_ptr, 8);
            *buf_ptr++ = ']';
            *buf_ptr++ = '[';
            buf_ptr += u32_to_text_padding(mydate.hour, buf_ptr, 2);
            *buf_ptr++ = ':';
            buf_ptr += u32_to_text_padding(mydate.minute, buf_ptr, 2);
            *buf_ptr++ = ':';
            buf_ptr += u32_to_text_padding(mydate.second, buf_ptr, 2);
            *buf_ptr++ = '.';
            buf_ptr += u32_to_text_padding(mydate.millisec, buf_ptr, 3);
            *buf_ptr++ = ']';
            output_file_write(log_file, buf, buf_ptr - buf);

            output_file_write(log_file, text, write_len);

            buf_ptr = buf;
            *buf_ptr++ = '#';
            buf_ptr += u64_to_text(order, buf_ptr);
            *buf_ptr++ = '\n';
            output_file_write(log_file, buf, buf_ptr - buf);
        }

    private:
        char log_file_path[log_path_max_length];
        outf log_file;
        volatile s64 busy;
    };

    template<typename ...Args>
    _INLINE_ void _log(const char* fmt, Args...args);
    _INLINE_ void _log(const char* fmt);

    template<typename ...Args>
    _INLINE_ void _err(const char* fmt, Args...args);
    _INLINE_ void _err(const char* fmt);

    class log_system
    {
        template<typename ...Args>
        friend _INLINE_ void _log(const char* fmt, Args...args);
        friend _INLINE_ void _log(const char* fmt);

        template<typename ...Args>
        friend _INLINE_ void _err(const char* fmt, Args...args);
        friend _INLINE_ void _err(const char* fmt);

    public:
        log_system() :
            path_buf(),
            workers(),
            order_id(0)
        {
            s64 path_buf_len = str_len(LOG_FOLDER);
            memory_copy(LOG_FOLDER, path_buf, path_buf_len);
            memory_copy("Log_", path_buf + path_buf_len, 4);
            path_buf_len += 4;

            date current_date = date_query();
            path_buf_len += s32_to_text(current_date.year, path_buf + path_buf_len);
            path_buf_len += u32_to_text_padding(current_date.month, path_buf + path_buf_len, 2);
            path_buf_len += u32_to_text_padding(current_date.day, path_buf + path_buf_len, 2);

            path_buf[path_buf_len++] = '_';

            path_buf_len += u32_to_text_padding(current_date.hour, path_buf + path_buf_len, 2);
            path_buf_len += u32_to_text_padding(current_date.minute, path_buf + path_buf_len, 2);
            path_buf_len += u32_to_text_padding(current_date.second, path_buf + path_buf_len, 2);

            path_buf[path_buf_len++] = '/';
            path_buf[path_buf_len] = 0;

            for (s64 i = 0; i < worker_count; ++i)
            {
                workers[i].init(path_buf, i);
            }
        }

        ~log_system()
        {
            char log_file_paths[worker_count][log_path_max_length];
            for (s64 i = 0; i < worker_count; ++i)
            {
                memory_copy(workers[i].mypath(), log_file_paths[i], log_path_max_length);
                workers[i].uninit();
            }
            merge_log(log_file_paths);
            for (s64 i = 0; i < worker_count; ++i)
            {
                delete_file(log_file_paths[i]);
            }
        }

    private:
        void write_log(const char* text, s64 text_len)
        {
            u64 myorder = atom_increment(order_id);
            s64 i = random(worker_count);
            for (s64 n = 0; n < worker_count * 2; ++n)
            {
                s64 select = (i + n) % worker_count;
                if (workers[select].try_get_control())
                {
                    workers[select].write_log(text, text_len, myorder);
                    workers[select].put_control();
                    return;
                }
            }
            assert_info(0, "log_workers are all busy, cannot write log.");
        }

        struct get_order_number_desc
        {
            s64 truncate_len;
            s64 prefix_len;
            s64 suffix_len;
            u64 order_number;
        };

        get_order_number_desc get_order_number(const char* text, s64 len)
        {
            get_order_number_desc desc = {};
            if (len == 0)
            {
                desc.order_number = u64_max;
                return desc;
            }
            s64 idx = 0;
            while (idx < len && text[idx] != '\n')
            {
                ++idx;
            }
            assert(idx < len);
            s64 from = idx - 1;
            while (from >= 0 && text[from] != '#')
            {
                --from;
            }
            assert(from >= 0);
            //# text[from : idx] is "#12345\n"
            desc.truncate_len = idx + 1;
            desc.suffix_len = idx - from + 1;
            desc.prefix_len = desc.truncate_len - desc.suffix_len;
            ++from;
            while (from < idx)
            {
                desc.order_number *= 10;
                desc.order_number += text[from++] - '0';
            }
            return desc;
        }

        void merge_log(char paths[worker_count][log_path_max_length])
        {
            s64 path_buf_len = str_len(path_buf);
            s64 ori_len = path_buf_len;
            path_buf[path_buf_len++] = 'l';
            path_buf[path_buf_len++] = 'o';
            path_buf[path_buf_len++] = 'g';
            path_buf[path_buf_len++] = '.';
            path_buf[path_buf_len++] = 't';
            path_buf[path_buf_len++] = 'x';
            path_buf[path_buf_len++] = 't';
            path_buf[path_buf_len] = 0;

            outf integrate_log_file = output_file_create(path_buf);
            path_buf_len = ori_len;
            path_buf[path_buf_len] = 0;

            char texts[worker_count][log_line_max_length];
            s64  text_lens[worker_count] = {};
            u64  text_order_numbers[worker_count] = {};
            inpf text_logs[worker_count];

            for (s64 i = 0; i < worker_count; ++i)
            {
                text_logs[i] = input_file_create(paths[i]);
                text_lens[i] = input_file_read(text_logs[i], texts[i], log_line_max_length);
                text_order_numbers[i] = get_order_number(texts[i], text_lens[i]).order_number;
            }
            while (1)
            {
                u64 least_order = u64_max;
                s64 least_order_idx = -1;
                for (s64 i = 0; i < worker_count; ++i)
                {
                    if (text_order_numbers[i] < least_order)
                    {
                        least_order = text_order_numbers[i];
                        least_order_idx = i;
                    }
                }
                if (least_order_idx == -1)
                {
                    break;
                }
                get_order_number_desc desc = get_order_number(texts[least_order_idx], text_lens[least_order_idx]);
                assert(desc.truncate_len);
                texts[least_order_idx][desc.prefix_len] = '\n';
                output_file_write(integrate_log_file, texts[least_order_idx], desc.prefix_len + 1);
                memory_copy(texts[least_order_idx] + desc.truncate_len, texts[least_order_idx], log_line_max_length - desc.truncate_len);
                text_lens[least_order_idx] -= desc.truncate_len;
                text_lens[least_order_idx] += input_file_read(
                    text_logs[least_order_idx], texts[least_order_idx] + text_lens[least_order_idx], desc.truncate_len);
                text_order_numbers[least_order_idx] = get_order_number(texts[least_order_idx], text_lens[least_order_idx]).order_number;
            }
            for (s64 i = 0; i < worker_count; ++i)
            {
                input_file_destroy(text_logs[i]);
            }
            output_file_destroy(integrate_log_file);
        }

    private:
        char path_buf[log_path_max_length];
        log_system_worker workers[worker_count];
        s64 order_id;
    };

    _SELECTANY_ log_system global_log_system;

    template<typename Ty>
    _INLINE_ void parse_args(char arg_buf[log_args_max_count][16], s64 arg_idx, Ty a)
    {
        memory_copy(&a, arg_buf[arg_idx], min(sizeof(Ty), 16));
    }

    template<typename Ty>
    _INLINE_ void parse_args(char arg_buf[log_args_max_count][16], s64 arg_idx, Ty* a)
    {
        memory_copy(&a, arg_buf[arg_idx], min(sizeof(Ty*), 16));
    }

    template<typename Ty, typename ...Args>
    _INLINE_ void parse_args(char arg_buf[log_args_max_count][16], s64 arg_idx, Ty a, Args...args)
    {
        memory_copy(&a, arg_buf[arg_idx], min(sizeof(Ty), 16));
        parse_args(arg_buf, arg_idx + 1, args...);
    }

    template<typename Ty, typename ...Args>
    _INLINE_ void parse_args(char arg_buf[log_args_max_count][16], s64 arg_idx, Ty* a, Args...args)
    {
        memory_copy(&a, arg_buf[arg_idx], min(sizeof(Ty*), 16));
        parse_args(arg_buf, arg_idx + 1, args...);
    }

    /*
     * %d    s32
     * %u    u32
     * %ll   s64
     * %ull  u64
     * %p    type*
     * %s    const char*
     */
    template<typename ...Args>
    _INLINE_ void _log(const char* fmt, Args...args)
    {
        assert_info(sizeof...(args) <= log_args_max_count, "Greater than max log arguements.");
        char output_buf[log_line_max_length];
        s64  output_len = 0;
        char arg_buf[log_args_max_count][16] = {};
        s64  arg_idx = 0;
        s64  len = str_len(fmt);
        s64  idx = 0;
        s64  idx_start = 0;
        parse_args(arg_buf, 0, args...);
        while (idx_start < len)
        {
            while (idx < len && fmt[idx] != '%')
            {
                ++idx;
            }
            memory_copy(fmt + idx_start, output_buf + output_len, idx - idx_start);
            output_len += idx - idx_start;
            if (idx >= len)
            {
                break;
            }
            if (fmt[idx + 1] == 's')
            {
                //# string
                char* arg_cursor = arg_buf[arg_idx++];
                const char* cstr = *pointer_convert(arg_cursor, 0, const char**);
                u64 cstr_len = str_len(cstr);
                assert(output_len + cstr_len < log_line_max_length);
                memory_copy(cstr, output_buf + output_len, str_len(cstr));
                output_len += cstr_len;
                idx_start = idx + 2;
                idx = idx_start;
            }
            else if (fmt[idx + 1] == 'p' || fmt[idx + 1] == 'x')
            {
                //# pointer
                char* arg_cursor = arg_buf[arg_idx++];
                u64 v = *pointer_convert(arg_cursor, 0, u64*);
                output_len += u64_to_text_hex(v, output_buf + output_len);
                assert(output_len < log_line_max_length);
                idx_start = idx + 2;
                idx = idx_start;
            }
            else if (fmt[idx + 1] == 'l' && fmt[idx + 2] == 'l')
            {
                //# s64
                char* arg_cursor = arg_buf[arg_idx++];
                s64 v = *pointer_convert(arg_cursor, 0, s64*);
                output_len += s64_to_text(v, output_buf + output_len);
                assert(output_len < log_line_max_length);
                idx_start = idx + 3;
                idx = idx_start;
            }
            else if (fmt[idx + 1] == 'u' && fmt[idx + 2] == 'l' && fmt[idx + 3] == 'l')
            {
                //# u64
                char* arg_cursor = arg_buf[arg_idx++];
                u64 v = *pointer_convert(arg_cursor, 0, u64*);
                output_len += u64_to_text(v, output_buf + output_len);
                assert(output_len < log_line_max_length);
                idx_start = idx + 4;
                idx = idx_start;
            }
            else if (fmt[idx + 1] == 'd')
            {
                //# s32
                char* arg_cursor = arg_buf[arg_idx++];
                s32 v = *pointer_convert(arg_cursor, 0, s32*);
                output_len += s32_to_text(v, output_buf + output_len);
                assert(output_len < log_line_max_length);
                idx_start = idx + 2;
                idx = idx_start;
            }
            else if (fmt[idx + 1] == 'u')
            {
                //# u32
                char* arg_cursor = arg_buf[arg_idx++];
                u32 v = *pointer_convert(arg_cursor, 0, u32*);
                output_len += u32_to_text(v, output_buf + output_len);
                assert(output_len < log_line_max_length);
                idx_start = idx + 2;
                idx = idx_start;
            }
            else
            {
                idx_start = idx;
                ++idx;
            }
        }
        output_buf[min(output_len, log_line_max_length - 1)] = 0;
        global_log_system.write_log(output_buf, output_len);
    }

    _INLINE_ void _log(const char* fmt)
    {
        global_log_system.write_log(fmt, str_len(fmt));
    }

    template<typename ...Args>
    _INLINE_ void _err(const char* fmt, Args...args)
    {
        assert_info(sizeof...(args) <= log_args_max_count, "Greater than max log arguements.");
        char output_buf[log_line_max_length] = "[ERROR] ";
        s64  output_len = 8;
        char arg_buf[log_args_max_count][16] = {};
        s64  arg_idx = 0;
        s64  len = str_len(fmt);
        s64  idx = 0;
        s64  idx_start = 0;
        parse_args(arg_buf, 0, args...);
        while (idx_start < len)
        {
            while (idx < len && fmt[idx] != '%')
            {
                ++idx;
            }
            assert(output_len + (idx - idx_start) < log_line_max_length);
            memory_copy(fmt + idx_start, output_buf + output_len, idx - idx_start);
            output_len += idx - idx_start;
            if (idx >= len)
            {
                break;
            }
            if (fmt[idx + 1] == 's')
            {
                //# string
                char* arg_cursor = arg_buf[arg_idx++];
                const char* cstr = *pointer_convert(arg_cursor, 0, const char**);
                u64 cstr_len = str_len(cstr);
                assert(output_len + cstr_len < log_line_max_length);
                memory_copy(cstr, output_buf + output_len, str_len(cstr));
                output_len += cstr_len;
                idx_start = idx + 2;
                idx = idx_start;
            }
            else if (fmt[idx + 1] == 'p' || fmt[idx + 1] == 'x')
            {
                //# pointer
                char* arg_cursor = arg_buf[arg_idx++];
                u64 v = *pointer_convert(arg_cursor, 0, u64*);
                output_len += u64_to_text_hex(v, output_buf + output_len);
                assert(output_len < log_line_max_length);
                idx_start = idx + 2;
                idx = idx_start;
            }
            else if (fmt[idx + 1] == 'l' && fmt[idx + 2] == 'l')
            {
                //# s64
                char* arg_cursor = arg_buf[arg_idx++];
                s64 v = *pointer_convert(arg_cursor, 0, s64*);
                output_len += s64_to_text(v, output_buf + output_len);
                assert(output_len < log_line_max_length);
                idx_start = idx + 3;
                idx = idx_start;
            }
            else if (fmt[idx + 1] == 'u' && fmt[idx + 2] == 'l' && fmt[idx + 3] == 'l')
            {
                //# u64
                char* arg_cursor = arg_buf[arg_idx++];
                u64 v = *pointer_convert(arg_cursor, 0, u64*);
                output_len += u64_to_text(v, output_buf + output_len);
                assert(output_len < log_line_max_length);
                idx_start = idx + 4;
                idx = idx_start;
            }
            else if (fmt[idx + 1] == 'd')
            {
                //# s32
                char* arg_cursor = arg_buf[arg_idx++];
                s32 v = *pointer_convert(arg_cursor, 0, s32*);
                output_len += s32_to_text(v, output_buf + output_len);
                assert(output_len < log_line_max_length);
                idx_start = idx + 2;
                idx = idx_start;
            }
            else if (fmt[idx + 1] == 'u')
            {
                //# u32
                char* arg_cursor = arg_buf[arg_idx++];
                u32 v = *pointer_convert(arg_cursor, 0, u32*);
                output_len += u32_to_text(v, output_buf + output_len);
                assert(output_len < log_line_max_length);
                idx_start = idx + 2;
                idx = idx_start;
            }
            else
            {
                ++idx;
                continue;
            }
        }
        output_buf[output_len] = 0;
        global_log_system.write_log(output_buf, output_len);
    }

    _INLINE_ void _err(const char* fmt)
    {
        char output_buf[log_line_max_length] = "[ERROR]";
        s64 output_len = 7;
        s64 strlen = str_len(fmt);
        assert(strlen + 7 < log_line_max_length);
        memory_copy(fmt, output_buf + output_len, strlen);
        output_len += strlen;
        output_buf[output_len] = 0;
        global_log_system.write_log(output_buf, output_len);
    }

}

#endif

template<typename ...Args>
_INLINE_ void log(Args...args)
{
    #if DEBUG_LEVEL >= DEBUG_LEVEL_CALIBRATION_LOG_NORM

    LoggerNs::_log(args...);

    #endif
}


template<typename ...Args>
_INLINE_ void err(Args...args)
{
    #if DEBUG_LEVEL >= DEBUG_LEVEL_CALIBRATION_LOG_ERROR

    LoggerNs::_err(args...);

    #endif
}

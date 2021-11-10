#pragma once

_INLINE_ ref<json_base> json_deserialize(const string& str, s64 from, s64 to)
{
    trim_index(str, from, to);

    if (from == to)
    {
        return ref<json_base>();
    }
    ref<json_base> rst = ref<json_base>();

    //===================================================== json_object
    rst = json_object::deserialize(str, from, to);
    if (rst.valid())
    {
        goto L_finish;
    }

    //===================================================== json_array
    rst = json_array::deserialize(str, from, to);
    if (rst.valid())
    {
        goto L_finish;
    }

    //===================================================== json_string
    rst = json_string::deserialize(str, from, to);
    if (rst.valid())
    {
        goto L_finish;
    }

    //===================================================== json_boole
    rst = json_boole::deserialize(str, from, to);
    if (rst.valid())
    {
        goto L_finish;
    }

    //===================================================== json_null
    rst = json_null::deserialize(str, from, to);
    if (rst.valid())
    {
        goto L_finish;
    }

    //===================================================== json_float
    rst = json_number::deserialize(str, from, to);
    if (rst.valid())
    {
        goto L_finish;
    }

    assert_info(0, "encounter unexpected charactor");
    return ref<json_base>();

L_finish:
    return rst;
}

_INLINE_ ref<json_base> json_deserialize(const string& str)
{
    return json_deserialize(str, 0, str.size());
}
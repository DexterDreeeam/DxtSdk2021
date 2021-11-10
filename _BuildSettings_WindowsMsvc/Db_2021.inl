/*
 *  I_AM_WINDOWS_MSVC
 *  I_AM_LINUX_GCC
 */
#define \
    I_AM_WINDOWS_MSVC

#define \
    SERVICE_NAME \
        "p9db"

/*
 *  Debug Level
 *    - DEBUG_FULL
 *        function call timer
 *
 *    - DEBUG_CRIT
 *        function call stack
 *        normal log
 *        error log
 *
 *    - DEBUG_HALF
 *        inside checker
 *        input checker
 *
 *    - DEBUG_NONE
 */
#define \
    DEBUG_CALIBRATION \
        DEBUG_FULL
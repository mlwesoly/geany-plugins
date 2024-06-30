AC_DEFUN([GP_CHECK_UTVC],
[
    GP_ARG_DISABLE([UTVC], [auto])
    GP_COMMIT_PLUGIN_STATUS([UTVC])
    AC_CONFIG_FILES([
        utvc/Makefile
        utvc/src/Makefile
    ])
])

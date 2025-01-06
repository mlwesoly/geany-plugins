AC_DEFUN([GP_CHECK_XTVC],
[
    GP_ARG_DISABLE([XTVC], [auto])
    GP_COMMIT_PLUGIN_STATUS([XTVC])
    AC_CONFIG_FILES([
        xtvc/Makefile
        xtvc/src/Makefile
    ])
])

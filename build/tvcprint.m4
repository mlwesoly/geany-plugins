AC_DEFUN([GP_CHECK_TVCPRINT],
[
    GP_ARG_DISABLE([TVCprint], [yes])
    GP_COMMIT_PLUGIN_STATUS([TVCprint])
    AC_CONFIG_FILES([
        tvcprint/Makefile
        tvcprint/src/Makefile
    ])
])

AC_DEFUN([GP_CHECK_PROJECTORGANIZER],
[
    GP_ARG_DISABLE([ProjectOrganizer], [no])
    GP_COMMIT_PLUGIN_STATUS([ProjectOrganizer])
    AC_CONFIG_FILES([
        projectorganizer/Makefile
        projectorganizer/src/Makefile
    ])
])

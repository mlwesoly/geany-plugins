AC_DEFUN([GP_CHECK_TVCTREEBROWSER],
[
    GP_ARG_DISABLE([TVCTreebrowser], [auto])
    GP_CHECK_UTILSLIB([TVCTreebrowser])

    if [[ "$enable_tvctreebrowser" != no ]]; then
        AC_CHECK_FUNC([creat],
            [enable_tvctreebrowser=yes],
            [
                if [[ "$enable_tvctreebrowser" = auto ]]; then
                    enable_tvctreebrowser=no
                else
                    AC_MSG_ERROR([Treebrowser cannot be enabled because creat() is missing.
                                  Please disable it (--disable-treebrowser) or make sure creat()
                                  works on your system.])
                fi
            ])
    fi

    AM_CONDITIONAL(ENABLE_TVCTREEBROWSER, test "x$enable_tvctreebrowser" = "xyes")
    GP_COMMIT_PLUGIN_STATUS([TVCTreeBrowser])

    AC_CONFIG_FILES([
        tvctreebrowser/Makefile
        tvctreebrowser/src/Makefile
    ])
])

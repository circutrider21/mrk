if win32 == true
  run_target('book',
    command : ['cmd', '/c',
	  'exec mdbook build -d "${MESON_BUILD_ROOT}/docs" "${MESON_SOURCE_ROOT}/${MESON_SUBDIR}"'
	    ]
    )
else
  run_target('book',
    command : ['sh', '-c',
	  'exec mdbook build -d "${MESON_BUILD_ROOT}/docs" "${MESON_SOURCE_ROOT}/${MESON_SUBDIR}"'
	    ]
    )
endif

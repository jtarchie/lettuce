add_rules("mode.debug", "mode.release")
add_requires("sqlite3")
add_includedirs("$(projectdir)/redcon.c/")

target("lettuce")
set_kind("binary")
add_files("src/main.c", "redcon.c/*.c")
add_packages("sqlite3")
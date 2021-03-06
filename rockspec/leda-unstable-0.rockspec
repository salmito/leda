package="leda"
version="unstable-0"

source= {
	url= "https://github.com/salmito/leda/archive/unstable.zip",
	dir= "leda-unstable"
}

description = {
  summary = "Leda",
  detailed = [[
    Leda is a Lua library for building parallel, non-linear pipelines in Lua based on
    the concepts of SEDA (Staged Event-Driven Architecture).
  ]],
  license = "MIT",
  homepage = "http://leda.co/"
}

dependencies = {
  "lua >= 5.1",
  "luasocket >= 2.0"
}

external_dependencies = {
  TBB = { library = "tbb", header="tbb/concurrent_queue.h" },
  LIBEVENT = { header="event2/event.h" },
}

build = {
   type = "make",
   platforms = {
      linux = {
         build_variables = {
            LDFLAGS = "-L$(TBB_LIBDIR) -L$(LIBEVENT_LIBDIR) -levent -ltbb -pthread -levent_pthreads",
            CFLAGS = "$(CFLAGS) -I$(LUA_INCDIR) -I$(TBB_INCDIR) -I$(LIBEVENT_INCDIR)",
            LUA="$(LUA)"
         },
         install_variables = {
            LUA_INSTALL_PATH = "$(LUADIR)",
            LIB_INSTALL_PATH = "$(LIBDIR)"
         },
         install_command = "make install"
       }
   }
}

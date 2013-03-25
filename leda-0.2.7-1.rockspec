package="leda"
version="0.2.7-1"

source= {
	url= "git://github.com/Salmito/leda.git",
	branch= "0.2.7"
}


description = {
  summary = "Leda",
  detailed = [[
    Leda is a Lua library for building parallel, non-linear pipelines based on
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
  LIBEVENT = { library = "event", header="event2/event.h" },
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

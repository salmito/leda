# Leda
Leda is a Lua library for bulding parallel, non-linear pipelines based on the concepts of the SEDA (Staged Event-Driven Architecture).

## Compiling and Installing
Leda is compatible with Lua 5.1 and 5.2.

Leda requires the Threading Building Blocks library (TBB). 

For more information on the tbb library: http://threadingbuildingblocks.org/
To install TBB on a Debian like linux (Ubuntu, mint, etc):
   apt-get install libtbb-dev

To build and install Leda for Lua 5.1:
```
   $ make
   $ sudo make install
```

To build and install Leda for Lua 5.2:
```
   $ make 5.2
   $ sudo make install5.2
```

To build and install Leda on both Lua versions:
```
   $ make clean
   $ sudo make install5.1
   $ make clean
   $ sudo make install5.2
```

To uninstall Leda on Lua 5.1:
```
   $ sudo make uninstall
```

To uninstall Leda on Lua 5.2:
```
   $ sudo make uninstall5.2
```

## Testing installation
To test if the installation was successful type this command:

```
$ lua -l leda
```

You should get the lua prompt if leda is installed properly or the error message "module 'leda' not found"  if it cannot be loaded (check if it was installed on a  wrong location).

That's it.

## Reporting BUGS
If you find bugs, please report them on GitHub: https://github.com/Salmito/Leda/issues

Or e-mail me: Tiago Salmito - tiago _[at]_ salmito _[dot]_ com
## Copyright notice
Leda is published under the same MIT license as Lua 5.1.

Copyright (C) 2012 Tiago Salmito, Noemi Rodriguez, Ana Lucia de Moura

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

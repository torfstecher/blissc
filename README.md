README for the BLISS-M Compiler
===============================

This is the source package for the BLISS-M compiler, a portable
cross-compiler for the BLISS programming language.
Visit the [project website](http://madisongh.github.io/blissc)
for more information about BLISS-M.

Current Status
--------------

Work in progress.   The front-end is mostly complete, with a fully
functioning parser and macro facility.  Back-end support is currently
limited to LLVM and x86 CPUs (32- or 64-bit), with a limited amount
of machine-specific support.

Prerequisites
-------------

Known to build on Mac OS X 10.8 (using Xcode 4.6) and Ubuntu 12.04
and 12.10 (32- or 64-bit).

The LLVM-based code generator was developed against LLVM 3.2, which
can be obtained [here](http://llvm.org/releases/download.html#3.2).
On a Mac, you can use the MacPorts 'llvm-3.2' package, but debugging
code generation issues is easier if you download the sources and
create a debug build yourself.  You should install it somewhere
and either include that location in your PATH, or specify the
full path to the `llvm-config` script that LLVM installs when
configuring the BLISS-M build.

The build system uses Autoconf and Automake.  On Mac OS X, you'll
need to install those (e.g., through MacPorts).

Building the compiler
---------------------

1. Clone [the repository](https://github.com/madisongh/blissc.git).

2. cd to the top-level source directory and run `autoreconf -i`
   to set up the autotools build.

3. If you want to build outside the source tree, cd to your
   build directory.

4. Run the `configure` script that was generated in step 2.  If
   `llvm-config` is not in your PATH, use the `--with-llvm-config`
   option on `configure` to specify its location.

5. Run `make` to build the compiler.

6. Run `make check` to test the built compiler.


Running the compiler
--------------------

The build will produce a program called **blissc** in your build
directory.  Run `./blissc --help` for a description of the arguments
and options.


Contributions
-------------

There is a lot more yet to do on this project!  If you are interested
in contributing, contact me (madisongh on GitHub, or by e-mail at
madison _at_ bliss-m _dot_ org).

License
-------
All sources are released under the BSD 2-clause license.  See the
[LICENSE.TXT](https://github.com/madisongh/blissc/blob/master/LICENSE.TXT)
file for the license text.

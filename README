TrueTime 2.0, 2016-04-06
========================

GETTING STARTED:

1. Unpack all files to some suitable directory $DIR

2. Start Matlab R2012a or later and cd to $DIR

3. Run init_truetime.m to add the necessary Matlab paths and to
   set the TTKERNEL environment variable.

4. Open any example, for instance $DIR/examples/simple/matlab/simple.slx
   Note that you must cd to the directory of the .slx file before you hit "Run".


RECOMPILING THE TRUETIME C++ MEX FILES:

The current release includes precompiled files for 64-bit Matlab under 64-bit
Windows, Mac OS X, and Linux. This should be sufficient to run all the examples
and also to construct new models that utilize the Matlab API.

If you are unable run the examples using the precompiled files, then you need
to recompile all the C++ MEX files:

a. Make sure that you have a Matlab-supported C++ compiler installed. See
   http://www.mathworks.com/support/compilers/current_release/

b. Run
   >> mex -setup C++

c. Run
   >> make_truetime
   and make sure that all files are compiled without error. You should now be
   able to run all the examples.


For questions and bug reports, please contact truetime@control.lth.se

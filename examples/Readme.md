The demo files include mostly all the features in ezl.
To learn about ezl, I suggest going by the following sequence:

0. wordcount and pi examples for overview.
1. 1D-diffusion for some recent features overview.
1. demoColumns. 
2. demoMapFilter.
3. demoFromFile.
4. demoReduce.
5. demoReduceAll.
6. demoPrll.
7. demoIO.
8. demoRise.
9. demoFlow.
and then examples of your choice.

Some of the examples are for illustration and do not write their results, while
some do show the results in output files or stdout. In any case you can append
`.dump()` to any unit(map/reduce/rise) to check the results and `.run()` at the end
of a data-flow to make it actually run. (`dump()` has two optional arguments, 
output file string and header of the file. Blank file-name implies stdout.)

I am hoping to write and record some basic stuff for a head start in modern c++ and
parallel data-processing with ezl.

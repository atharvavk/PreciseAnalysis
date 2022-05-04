<h1>Running Instructions:</h1>
(This setup was done for arch linux)<br/>
LLVM setup is pre-requisite for this application<br>
Set LLVM_HOME and LLVM_DIR paths in your repository as:<br>

export LLVM_HOME=/usr/lib/<br>
export LLVM_DIR=/usr/include/llvm<br>
<br>
(Z3 package needs to be installed globally)
<br>
Execute <br>
./build.sh<br>
<br>
Execute<br>
cmake<br>
make<br>
<br>
Whole process will take about 30 min to compile.<br>
After that include and lib folders need to be moved to global include and lib sections<br>
cp -r include/\* /usr/local/include/<br>
cp -r lib/\* /usr/local/lib<br>
<br>
After any changes in tools/Example execute make.<br>

<br>
To get detailed analysis of CHA and VSFS:<br>
execute:<br>
bin/svf-ex /path/to/bitcode/file<br>
<h2>Citations:</h2> 

```
@inproceedings{sui2016svf,
  title={SVF: interprocedural static value-flow analysis in LLVM},
  author={Sui, Yulei and Xue, Jingling},
  booktitle={Proceedings of the 25th international conference on compiler construction},
  pages={265--266},
  year={2016},
  organization={ACM}
}
```

```
@article{sui2014detecting,
  title={Detecting memory leaks statically with full-sparse value-flow analysis},
  author={Sui, Yulei and Ye, Ding and Xue, Jingling},
  journal={IEEE Transactions on Software Engineering},
  volume={40},
  number={2},
  pages={107--122},
  year={2014},
  publisher={IEEE}
}
```





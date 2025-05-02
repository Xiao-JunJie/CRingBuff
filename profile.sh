###
 # @Description: Copyright Xiao
 # @Autor: Xjj
### 

sudo perf record -F 99 -g --call-graph dwarf ./main
perf script | FlameGraph/stackcollapse-perf.pl | FlameGraph/flamegraph.pl > flamegraph.svg

echo "火焰图已生成：flamegraph.svg"
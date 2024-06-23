#! /usr/bin/env sh

bench_str='
bench()
{
    for _ in {1..400}
    do
        _before_command
        _after_command
    done
}
bench
'

time -f "Bash: %e s" bash -ic "$bench_str"
time -f "Zsh: %e s" zsh -ic "$bench_str"

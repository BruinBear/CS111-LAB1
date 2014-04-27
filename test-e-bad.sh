#! /bin/sh

# UCLA CS 111 Lab 1b - Test bad execution of the script

tmp=$0-$$.tmp
mkdir "$tmp" || exit
(
cd "$tmp" || exit
status=
n=1

for bad in \
  '`' \
  '>' \
  '<' \
  'echo hello >foo <' \
  ';' \
  '; ' \
  'ls . ||' \
  'echo hello
     || echo world' \
  'echo whats
     | echo up' \
  'echo hi
     ; echo there' \
  'echo Its;;echo nice' \
  'echo you&&&echo failed' \
  'echo ls|||ls .' \
  '|echo true' \
  '< echo indir' \
  '&& echo and' \
  '||echo or' \
  '(echo a|echo pipe' \
  'echo semi;echo comma)' \
  '( (echo sub)' \
  'echo re>>>echo'
do
  echo "$bad" >test$n.sh || exit
  ../timetrash  test$n.sh >test$n.out 2>test$n.err && {
    echo >&2 "test$n: unexpectedly succeeded for: $bad"
    status=1
  }
  test -s test$n.err || {
    echo >&2 "test$n: no error message for: $bad"
    status=1
  }
  n=$((n+1))
done

exit $status
) || exit

rm -fr "$tmp"


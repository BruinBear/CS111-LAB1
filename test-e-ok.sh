#! /bin/sh

# UCLA CS 111 Lab 1b - Test normal execution of the script

tmp=$0-$$.tmp
mkdir "$tmp" || exit

(
cd "$tmp" || exit

cat >test1.sh <<'EOF'
true


cat < /etc/passwd | tr a-z A-Z | sort -u || echo sort failed!


cat < /etc/passwd | tr a-z A-Z | sort -u > out || echo sort failed!

echo 1 && echo 2||
 echo 3 &&
  echo 4 | echo 5 && echo 6|

echo 7


#output hello 				passed
echo hello

#should do both				passed
echo hello && ls ..

#should only output hello		passed
echo hello || ls ..

#IO					passed
cat < ../README > tmp

#sequencial command			passed
rm tmp;   echo world ; ls ..

#subshell not output dude		passed 
(echo whats) && (echo up) || (echo dude)
echo whats && (echo up) || (echo dude)

#pipeline				passed
cat < ../README | sort | grep the

#white		spaces			passed
(cat)	<   ../README  | 
sort 	|	
	grep the || 
			echo last test failed
EOF

/bin/bash test1.sh > bash.out
../timetrash test1.sh >trash.out
diff -u trash.out bash.out || exit


) || exit

rm -fr "$tmp"


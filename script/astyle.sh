#/bin/sh

base=$(dirname $0)

for f in $(find $base/../src -name "*.cc" -o -name "*.hh" ); do
    astyle -n --style=stroustrup --indent=spaces=4 \
        --break-closing-brackets --break-blocks --align-pointer=type \
        --add-brackets --pad-paren-in < $f > astyle.tmp.out
    cp astyle.tmp.out $f
done

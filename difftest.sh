testa3.sh explain $1 | awk 'NR==8' > ./.testthis
chmod 750 ./.testthis
./.testthis
echo -n 'diff -y ' > ./.diffthis
testa3.sh explain $1 | grep ".err tests" | cut -d\  -f 2- >> ./.diffthis
chmod 750 .diffthis
./.diffthis
echo -n 'r ' > ./dsl.conf
cut -d\  -f 2- .testthis | rev | cut -d\  -f 5- | rev >> ./dsl.conf

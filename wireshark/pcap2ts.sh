# /usr/bin/zsh

sudo chmod 444 $2
if [ -f $2 ]; then
  echo "File $2 exists and permissions have been set to 444."
else
  echo "File $2 does not exist."
fi

file=$2
python_script=$1
base=${file%.*}
tmp=$base.tmp
csv=$base.csv
log=$base.log

tshark -r $2 -t e | grep Cmds > $tmp
head -n -2 $tmp > /dev/null
sed -i '1, 4d' $tmp > /dev/null
awk '{print $2}' $tmp > $csv 
python3 $1 $csv > /dev/null
rm $tmp
mv $log ../log/ts/


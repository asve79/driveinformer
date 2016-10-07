#!/bin/bash

#set -x

#- Создаем каталог если не существует
here=`pwd`
if [ ! -d send ]; then
 mkdir send
fi

#- проверим флаг залочки. Если он будет установлен, то выходим
#- например когда мы в роуминге

if [ -f $0.lock ]; then
 exit 1
fi

#- Пробежимся по путям с файлами (логи и треки)
for i in logs tracks; do
 cd $here/$i
 cnt=`ls -1 | wc -l`
 if [ $cnt -ne 0 ]; then
  for j in *; do
   res=`/bin/fuser $j 2>/dev/null`
   if [ "$res" == "" ];then
    mv $j $here/send
   fi
  done
 fi
done

#- Теперь посмотрим нужно ли что-то отправлять
cd $here/send
cnt=`ls -1 | wc -l`
if [ $cnt -eq 0 ];then
 exit 0
fi

#- Если нужно отправлять, то сначала проверям доступность ресурса
while [ 1 ];do
 curl http://git.ae-nest.com >/dev/null 2>/dev/null
 if [ $? -ne 0 ]; then
  sleep 60  #- если ссылка не открылась то ждем минуту
  continue
 fi

#- Проверим нужно ли что-то отправлять
 cnt=`ls -1 | wc -l`
 if [ $cnt -gt 0 ]; then
  for i in *; do
   scp -P 8022 $i ae-nest.com:/home/pi/inbox
   if [ $? -eq 0 ]; then
    rm $i
    sleep 1
   else
    #- если во время отправки прозошла ошибка то выходим из цикла.
    #- возможно плохая связь
    break
   fi
  done
 fi

 #- проверим все ли отправили, если все то выходим из вечного цикла
 cnt=`ls -1 | wc -l`
 if [ $cnt -eq 0 ]; then
  break
 fi

 #- если отправили не все, то ждем минуту и отправляем заново
 sleep 60
done


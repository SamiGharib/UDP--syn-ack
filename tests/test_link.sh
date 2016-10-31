#!/bin/bash
#Clean up 
rm -f *.out *.log *.diff

cleanup(){
		kill -9 $receiver_pid
		kill -9 $simlink_pid
		exit 0
}
trap cleanup SIGINT #Kill les process en arrière plan en cas de ^-C
./tests/link_sim -p 1341 -P 2456 -l 10 -d 10 -R &> link.log &
simlink_pid=$!&
echo "Startint test w/ sim link"
# Itération sur les fichiers d'entrée
for filename in tests/*.in; do
		echo "start test for $filename"
		fileout=$( basename "$filename" .in).out
		./receiver -f "$fileout" ::1 2456 2> receiver.log &
		receiver_pid=$!
		
		if ! ./sender ::1 1341 < "$filename" 2> sender.log ; then
				echo "Crash du sender"
				cat sender.log
				exit 1
		fi

		sleep 5

		if kill -0 $receiver_pid &> /dev/null ; then
				echo "Le receiver ne s'est pas arreté à la fin du transfert!"
				kill -9 $receiver_pid
				err=1
		else
				if ! wait $receiver_pid ; then
						echo "Crash du receiver"
						cat receiver"$base".log
						exit 1
				fi
		fi
		if [[ "$(md5sum $filename | awk '{print $1}')" != "$(md5sum $fileout | awk '{print $1}')" ]]; then
		  echo "Le transfert a corrompu le fichier $filename !"
		  echo "Diff binaire des deux fichiers: (attendu vs produit)" 
		  diff -C 9 <(od -Ax -t x1z "$filename") <(od -Ax -t x1z "$fileout")
		  exit 1
		else
		  echo "Le transfert est réussi!"
		fi
done
exit 0

#!/bin/bash
#Clean up 
rm -f *.out *.log *.diff

cleanup(){
		kill -9 $receiver_pid
		exit 0
}
trap cleanup SIGINT #Kill les process en arrière plan en cas de ^-C
# Itération sur les fichiers d'entrée
echo "Starting test without sim link"
for filename in tests/*.in; do
		echo "start test for $filename"
		fileout=$( basename "$filename" .in).out
		./receiver -f "$fileout" ::1 1234 2> receiver.log &
		receiver_pid=$!
		
		if ! ./sender ::1 1234 < "$filename" 2> sender.log ; then
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

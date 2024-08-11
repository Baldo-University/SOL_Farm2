IMPORTANTE
La opendir alla riga 286 (piu' o meno) vuole il filepath relativo
Nella versione corrente 18:51 11/8/24 viene passato solo il nome della cartella, senza il filepath
SOLUZIONE: rifare la list_add_tail aggiungendo un parametro di path
tanto questa fz. si usa solo per l'aggiunta alla lista di directory, nessun altro vorra' usarla 

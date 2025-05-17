#!/bin/bash

SERVER_EXEC=./bin/dserver
CLIENT_EXEC=./bin/dclient
DOC_FOLDER=data/documents
CACHE_SIZE=10
RESULTS_FILE=scripts/results.txt

echo "===== TESTE DE PESQUISA SEQUENCIAL VS PARALELA =====" > "$RESULTS_FILE"
echo "Iniciando servidor..." | tee -a "$RESULTS_FILE"
$SERVER_EXEC $DOC_FOLDER $CACHE_SIZE &
SERVER_PID=$!
sleep 1

echo "Indexando documentos..." | tee -a "$RESULTS_FILE"
for file in $DOC_FOLDER/*.txt; do
    $CLIENT_EXEC -a "Título" "Autor" 2024 "$file" > /dev/null
done

sleep 1

echo "START (tempo em segundos)" | tee -a "$RESULTS_FILE"

# SEQUENCIAL
for i in 1 2 3; do
    START=$(date +%s.%N)
    $CLIENT_EXEC -s "the" > /dev/null
    END=$(date +%s.%N)
    echo "Sequencial ($i): $(echo "$END - $START" | bc)" | tee -a "$RESULTS_FILE"
done

# PARALELO
for i in 1 2 3; do
    START=$(date +%s.%N)
    $CLIENT_EXEC -s "the" 4 > /dev/null
    END=$(date +%s.%N)
    echo "Paralelo ($i): $(echo "$END - $START" | bc)" | tee -a "$RESULTS_FILE"
done

echo "Pedido de encerramento enviado ao servidor." | tee -a "$RESULTS_FILE"
$CLIENT_EXEC -f
    sleep 1
    kill $SERVER_PID 2>/dev/null
    wait $SERVER_PID 2>/dev/null

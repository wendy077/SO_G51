#!/bin/bash

SERVER_EXEC=./bin/dserver
CLIENT_EXEC=./bin/dclient
DOC_FOLDER=data/documents
RESULTS_FILE=scripts/results_cache.txt

echo "===== TESTE DE IMPACTO DO TAMANHO DA CACHE =====" > "$RESULTS_FILE"

for CACHE_SIZE in 1 2 4 8 16; do
    echo "Cache size = $CACHE_SIZE" | tee -a "$RESULTS_FILE"
    
    $SERVER_EXEC $DOC_FOLDER $CACHE_SIZE &
    SERVER_PID=$!
    sleep 1

    for file in $DOC_FOLDER/*.txt; do
        $CLIENT_EXEC -a "Título" "Autor" 2024 "$file" > /dev/null
    done

    sleep 1

    for i in 1 2 3; do
        START=$(date +%s.%N)
        $CLIENT_EXEC -s "the" > /dev/null
        END=$(date +%s.%N)
        echo "  Execução $i: $(echo "$END - $START" | bc)" | tee -a "$RESULTS_FILE"
    done

    $CLIENT_EXEC -f
    sleep 1
    kill $SERVER_PID 2>/dev/null
    wait $SERVER_PID 2>/dev/null

    echo "----------------------------------" | tee -a "$RESULTS_FILE"
done

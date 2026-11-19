# OpenDataLogger v0.4.6

- Matriz LED digital utiliza directamente os flancos D2/D3 e simula os impulsos por eventos, em vez de amostrar `latest`.
- Matriz LED analógica utiliza os registos GPT reais do historico circular.
- Removido o segundo historico `MatrixPoint` e a sua reserva de RAM.
- ODLB v2 expõe `deviceNowMs` nos bytes 58..61, que alinha o eixo temporal dos graficos Web com o dispositivo.
- Mantidos trigger, aquisicao GPT, fila, API binaria, perfis de canais e modos de visualizacao.
- Ensaiar na UNO R4 WiFi: sinal A0 simulado, valvula 250ms em janelas 3s e 10s; verificar FIFO e jitter.
- Ainda nao compilado com as bibliotecas oficiais Renesas; teste de hardware por realizar.

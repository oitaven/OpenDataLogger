# OpenDataLogger 0.4.7 — continuidade de aquisição e visualização

- **Aquisição GPT e FIFO mantidas** (sem alterações à ISR): a prioridade continua a ser recolher as amostras.
- API de controlo com respostas JSON pequenas: `ok`, `matrix`, `matrixWindowMs`; no trigger inclui também `trigger`.
- `/api/status` e `/api/diagnostics` usam agora um buffer HTTP de 256 bytes em vez de centenas de `print()` isolados.
- Durante envios de blocos, o firmware tenta actualizar a matriz entre escritas. A transferência da página inicial descarrega também a FIFO entre blocos.
- Alterações a nomes, cores, calibração/escala ou valores de um canal **sem mudar o seu tipo** não param nem reiniciam a aquisição nem limpam o histórico. Se mudar o tipo de canal, o histórico continua a ser reiniciado para evitar interpretações incorrectas.
- A matriz analógica mostra no máximo um LED por coluna: médias por coluna e interpolação *só na representação visual*, para diminuir o serrilhado. A aquisição e os ficheiros de dados mantêm as leituras originais.
- A reconstrução do estado inicial digital na Web passa a usar a transição seguinte, para evitar discrepâncias em D2 quando não existe um flanco anterior na janela.
- Modo de visualização inicial: **Gráfico único · faixas independentes**.

## Importante

A matriz é de 12×8 LEDs; uma curva analógica *não* pode ser perfeitamente contínua nessa resolução. O movimento pode continuar a sofrer pausas se uma operação `WiFiClient::write` ou `flush()` bloquear o `loop()`; a interrupção GPT continua a executar enquanto possível, e o contador de perdas FIFO deve permanecer em zero. A reprodução de estados digitais pelo histórico é limitada aos 128 eventos armazenados.

**Testes de software não equivalem a compilação Renesas real nem a ensaio em hardware.** Primeiro testar em simulação e confirmar os diagnósticos na UNO R4 WiFi.

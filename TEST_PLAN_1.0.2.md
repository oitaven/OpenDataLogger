# Plano de ensaio — 1.0.2

Estado: este documento é uma proposta de testes, **não uma declaração de que foram realizados**.

## Preparação
- Seleccionar **Arduino UNO R4 WiFi**, núcleo **Arduino UNO R4 Boards 1.6.0**.
- Criar `arduino_secrets.h` a partir de `arduino_secrets.h.example` e definir credenciais locais.
- Guardar firmware funcional anterior para recuperação.

## Compilação
- `Verify` no Arduino IDE ou sucesso do workflow `Compilar para Arduino UNO R4 WiFi` no GitHub Actions.
- Confirmar memória ocupada e avisos do compilador.

## Aquisição
- Em DEMO, comparar intervalos ADC de 100, 250, 500 e 1000 ms; verificar perdas FIFO e jitter.
- Em canal digital, confirmar impulsos de 250 ms com ADC a 10 s, independentemente da taxa ADC.
- Repetir mudanças de separadores, nomes e modos gráficos; confirmar ausência de perdas e tempos medidos.
- Confirmar que a saída Web usa o protocolo ODLB v2 e aceita reconexões.

## Matriz LED
- A1 DEMO: testar janelas 10 e 30 s; a sinusoide tem período de **9,5 segundos**.
- A janela de 30 s representa >3 ciclos em 12 colunas e não permite uma sinusoide suave; documentar a resolução (2,5 s por coluna).
- Testar D2/D3 e detectar flancos, com 1, 3 e 10 s.

## Configuração e persistência
- Verificar a aplicação de valores por omissão e a prevalência da EEPROM.
- Testar calibração NTC apenas no modo AQUISIÇÃO REAL, com referência estável.
- Guardar credenciais Wi-Fi pela Web apenas em LAN privada; verificar reinício e fallback AP.
- Confirmar início/paragem de sessão no navegador e exportação/importação.
- Caso exista microSD, confirmar detecção e integridade CSV; ensaiar falhas de energia. Nunca ligar cartão 3,3 V sem adaptação ao Arduino.

## Critérios para chamar uma versão «estável»
- Compilação automática verde, ensaios físicos documentados, perdas/jitter compatíveis com os requisitos, testes de segurança de configuração e comportamento perante falhas.

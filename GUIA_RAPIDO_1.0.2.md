# Guia rápido — OpenDataLogger 1.0.2

## DEMO e AQUISIÇÃO REAL

Em **Monitorização → Aquisição** existem dois botões destacados:
- **AQUISIÇÃO REAL**: lê as entradas físicas A0–A5 e os flancos de D2–D3; é o modo no arranque.
- **DEMO**: os sinais são gerados pelo Arduino. A0 faz uma onda triangular; A1 uma sinusoide aproximada (LUT de 64 pontos); D2 uma onda quadrada; D3 impulsos curtos.

Mudar de modo reinicia o histórico circular do Arduino. Uma sessão do Safari em gravação pede confirmação antes da mudança e é parada para evitar misturar origens distintas.

## Aquisição, sessão e análise — são coisas diferentes

1. **Aquisição em curso**: o Arduino está a recolher entradas com o intervalo seleccionado. O temporizador GPT tem prioridade. O modo pode ser DEMO ou REAL.
2. **Sessão do navegador**: em **Sessões → Iniciar sessão / Parar sessão**, acumulas amostras no Safari; parar esta gravação **não** pára a aquisição do Arduino. Exporta-a antes de fechar a página.
3. **Análise**: em **Análise → Calcular relatório** processas as amostras e flancos disponíveis, sem interromper a aquisição. Se existir uma sessão do Safari com dados, será utilizada; caso contrário, o histórico circular do Arduino.
4. **Trigger → Parar/congelar**: congela a captura no Arduino. Para voltar à aquisição contínua, escolhe **Contínuo**.

## Configuração inicial dos canais

`ChannelDefaults.h` contém oito perfis iniciais (nome, cor, tipo, escala, ganho, offset, parâmetros NTC). Os valores só são aplicados se não houver configuração válida na EEPROM. Não substituem definições previamente guardadas.

## Calibrar uma NTC

1. Em **Canais**, altera o tipo de A0–A5 para `NTC / temperatura`, define R fixa, R25 e beta, e regressa a Monitorização para aplicar. Os valores devem corresponder ao circuito real.
2. Escolhe **AQUISIÇÃO REAL**. Não se pode calibrar usando valores simulados.
3. Coloca a sonda junto de um termómetro de referência, espera pela estabilização e considera usar 100–500 ms durante esta operação para a última leitura ADC ser recente.
4. Em **Análise → Calibração NTC multiponto**, escreve a temperatura que mediste com o termómetro e usa **Guardar ponto da leitura actual**.
5. Repetir em 1 a 3 temperaturas distintas. Um ponto corrige desvio, dois interpolam a correcção, três calculam Steinhart–Hart.
6. Em **Monitorização**, clica em **Guardar configuração**, para persistir na EEPROM.

O botão **Calibrar NTC** em Canais é uma alternativa de **correcção rápida de um ponto** (trim), não a mesma coisa que a calibração multiponto. Evita acumular correcções sem controlo.

## microSD

A biblioteca Arduino SD usada por este projecto lê FAT16/FAT32, não exFAT. Usa um cartão microSD até 32 GB, preferencialmente formatado FAT32 no computador (macOS → Utilitário de Discos → Apagar → MS-DOS (FAT), esquema MBR), depois de guardar tudo o que for importante.

Não há botão de formatação remoto: poderia apagar inadvertidamente ficheiros e exigir maior complexidade de recuperação. Se o microSD estiver ligado no arranque, o firmware tenta detectá-lo e iniciar sessão; sem cartão, continua em RAM e no Safari. A ligação eléctrica do leitor deve ser validada antes do uso.

## Actualizar a pasta do Arduino

Copia **todos os ficheiros `.ino` e `.h` do ZIP** para a mesma pasta `OpenDataLogger` no Mac, incluindo **`AdvancedFeatures.h`**, **`OptionalSD.h`** e **`ChannelDefaults.h`**. Preserva o teu `arduino_secrets.h`, caso contenha credenciais reais. O Arduino IDE pode manter ficheiros antigos abertos: confirma que recompilas a pasta actualizada.

A compilação integral no Arduino IDE e a validação eléctrica dos sensores continuam a ser necessárias.

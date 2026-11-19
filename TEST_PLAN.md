# Ensaio recomendado — versão 0.4.5

## Antes de instalar

1. Guardar a pasta da 0.4.4a, para facilitar o regresso.
2. Preencher `arduino_secrets.h` com o SSID e a palavra-passe pretendidos, mantendo a palavra-passe do AP com pelo menos oito caracteres.
3. Compilar no **Arduino IDE 2**, placa **UNO R4 WiFi**, núcleo Renesas 1.6.0 ou versão compatível, e fazer Upload. Não houve compilação real com esse núcleo no ambiente de desenvolvimento desta revisão.
4. Evitar qualquer ligação a 230 V nesta fase. Os ensaios em modo de simulação dispensam sensores e resistências.

## Teste decisivo

1. Abrir o endereço IP do Arduino no Safari; seleccionar **Simulação** e manter D2 (Compressor) e D3 (Válvula) ativos.
2. Seleccionar aquisição a **10 segundos**. Escolher **Gráfico único · faixas por canal** e janela automática ou pelo menos 30 s.
3. Aguardar alguns períodos do simulador. A válvula deve mostrar um pulso de ~250 ms a cada 8 s, mesmo que os valores amostrados do canal D3 estejam maioritariamente OFF.
4. Mudar a aquisição para **1 segundo**, depois **100 ms**, sem alterar a cadência de impulsos visíveis. Os valores analógicos mostram maior ou menor detalhe conforme a taxa, mas os flancos devem permanecer.
5. Escolher **CSV impulsos** e verificar duas linhas (ON/OFF) por pulso, com duração aproximada de 250 000 µs.
6. Abrir **Sistema** e observar contadores de transições e perdas; verificar a fila de aquisição, o tempo máximo de resposta HTTP e `GPT (prioritário)`.

## Teste real de 1 ms, apenas em baixa tensão

Com gerador de impulsos 0–5 V e massa comum corretamente ligados a D2 ou D3, medir um pulso de 1 ms. Não usar rede de 230 V, nem qualquer entrada não protegida. Repetir com aquisição a 10 segundos. **A deteção de todos os impulsos não está garantida até medirmos prioridades, duração da ISR e erros de captura com equipamento de referência.**

## Testes realizados neste ambiente

- Compilação C++ com simulações de bibliotecas Arduino: **passou**, mas não é uma compilação Renesas real.
- Simulador: evento de válvula com 250 ms preservado a 10 segundos de amostragem: **passou**.
- Transição real simulada com 1 ms: **passou**.
- Memória circular de 128 transições, protocolo ODLB v2 completo/incremental/sem novidades: **passou**.
- Sintaxe JavaScript e cinco modos gráficos em Chromium: **passou**.

## Limitações conhecidas

- Histórico em RAM perde-se com reinício ou falta de energia; ainda não existe microSD.
- Os flancos usam um anel de 128 eventos; quando ultrapassado, perdem-se os mais antigos. A exportação CSV só inclui os dados que o navegador ainda tem em memória.
- A simulação matemática não substitui testes eléctricos ou medições de latência no microcontrolador.
- A configuração de canais ainda reinicia o histórico em RAM; está prevista uma política de preservação selectiva para a próxima fase.

## 0.4.6 — Mesmo eixo temporal na Web e na matriz

1. Com o modo **Simulação** ligado e **100 ms** de aquisição, escolher **Sensor A0** na matriz e janela **1 s**. Deve surgir um pequeno segmento quase recto; perto dos vértices da onda triangular haverá naturalmente mudança de declive. A matriz utiliza os registos analógicos temporizados, não o ultimo valor em cache.
2. Escolher **Válvula** e matriz **3 s**, depois **10 s**. Confirmar que cada impulso ON de cerca de 250 ms fica visível durante a passagem pela janela e não desaparece só por ficar OFF dentro da mesma coluna. Na janela de 10 s (833 ms por coluna), o impulso cabe num único traço vertical — a resolução física não permite desenhar 250 ms como largura horizontal perceptível.
3. Manter o digital na matriz, alterar o intervalo de aquisição para **1 s** e depois **10 s**. Os flancos devem continuar visíveis, porque são guardados separadamente.
4. Comparar o Safari e a matriz. Ambos usam a origem temporal do Arduino; a Web pode estar atrasada pelo tempo do Wi-Fi e de desenho. O campo `deviceNowMs` permite avançar a janela do navegador nos períodos sem amostras novas.
5. Verificar em **Sistema**: GPT activo, FIFO sem perdas e jitter real. Repetir após trigger congelado: o gráfico deve permanecer parado.

### O que foi testado aqui

- Compilação C++ com bibliotecas Arduino simuladas (não é compilação Renesas real).
- Teste de matriz com subida/descida de 250 ms em janela de 10 s, aquisição principal a 10 s; a coluna do impulso manteve-se acesa em vários instantes de desenho.
- Teste de A0 com amostras de 100 ms: ao adulterar `latest[]` o gráfico não se alterou.
- Teste de captura congelada: desenhar com relógio vários segundos mais tarde não alterou o bitmap.
- Formato ODLB v2 e cinco modos da página no Chromium.

### Limitações

A resolução da matriz é apenas **12 × 8**. A representação de impulsos curtos será necessariamente aproximada; os carimbos temporais completos ficam no registo de eventos e nos gráficos Web. Com a taxa ADC inferior à velocidade de uma alteração analógica, não é possível inventar amostras em falta. A verificação física na UNO R4 WiFi continua necessária.

## Ensaio suplementar — 0.4.7 (prioridade da aquisição)

1. **Segurança**: começar com simulação, não ligar os circuitos de 230 V à placa.
2. **Vista inicial**: após abrir a página, deve aparecer «Gráfico único · faixas independentes».
3. **A0 / 1 segundo**: aquisição a 100 ms, matriz 1 s. Deve aparecer um traço de 1 LED por coluna, geralmente inclinado e quase rectilíneo fora dos vértices da onda triangular do simulador. Uma matriz 12×8 nunca será um osciloscópio analógico de resolução contínua.
4. **Válvula / 3 e 10 segundos**: os impulsos simulados de 250 ms devem continuar visíveis, mesmo quando alteramos a taxa de amostragem analógica; em 10 s o impulso ocupa tipicamente apenas uma coluna.
5. **Compressor D2 / 10 segundos**: comparar o início e o fim das transições com a linha no Safari. Usar janelas temporais equivalentes; a Web tem latência de rede e a matriz utiliza apenas 12 colunas.
6. **Interacções Web**: mudar o tipo de gráfico, canal na matriz, janela 1/3/10 s, e nome/cor/escala de um canal. A matriz deve tentar continuar a desenhar entre blocos HTTP e a aquisição GPT deve continuar; alterações do tipo do canal, taxa de amostragem, modo demo, trigger e gravação na EEPROM podem implicar reinícios/pausas intencionais.
7. **Sistema**: confirmar `GPT (prioritário)`, perdas FIFO=0, períodos IRQ saltados=0. Se esses valores subirem, guardar captura do painel e explicar qual a operação da página em curso.
8. **Validação C++**: compilação feita com simuladores das bibliotecas (não com a toolchain Renesas real); confirmar Verify e Upload no Arduino IDE antes de ligar sensores reais.

**Limitações conhecidas:** a matriz continua servida pelo `loop()`. Uma chamada bloqueante individual de `WiFiClient::write()`/`flush()`/`stop()` ainda a pode deixar temporariamente parada. Um servidor HTTP inteiramente não-bloqueante requer uma futura refactorização da ligação TCP. Leituras analógicas dentro da ISR GPT permanecem experimentais e exigem validação física.

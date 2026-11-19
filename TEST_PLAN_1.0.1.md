# Plano de validação física — OpenDataLogger 1.0.1

## A. Primeiro arranque sem cartão microSD

1. Guardar 0.4.7 como versão de recuperação, configurar `arduino_secrets.h` e `Verify/Upload` no Arduino IDE 2, núcleo Renesas 1.6.0.
2. Com simulação e 100 ms, confirmar `Sistema → Desempenho`: modo GPT, FIFO sem perdas, desvio de IRQ reduzido; registar os valores ao fim de 5, 30 e 60 min.
3. Observar A0 na matriz 1/3/10 s; D2 e D3 em 3/10 s. Confirmar que os 250 ms ON do canal D3 continuam disponíveis a 1 s e 10 s de amostragem analógica (eventos assíncronos).
4. Mudar vista, cor, nome, escala e canal virtual, verificando que as amostras continuam a surgir e que a FIFO não apresenta perdas.
5. Introduzir 4 regras: canal A0 acima do limiar; ON de D2 por 2 s; A0−A1 acima de limite; D2 ON E A0 acima de limite. Confirmar o registo de cada ocorrência. Armar trigger e experimentar uma regra com acção de disparo.

## B. Sessões e gráficos

1. Clicar Iniciar sessão no Safari, adquirir 2 a 5 minutos, parar, exportar JSON. Importar o ficheiro, arrastar a posição da reprodução e comparar com o histórico actual.
2. Seleccionar cursores A/B num gráfico, comparar Δt com o período do simulador e Δvalores com os valores exportados em CSV.
3. Abrir `EXEMPLO_SESSAO_DEMO.json` (ficheiro sintético de testes), comparar A0/D2 e alinhar pelo primeiro flanco digital de subida.
4. Desligar Wi-Fi durante mais do que a retenção ADC e verificar o aviso de lacuna; confirmar que o sistema não preenche falsamente as amostras que faltam.

## C. Calibração e Wi-Fi

1. Num sensor NTC real isolado, registar 3 temperaturas de referência estáveis, suficientemente separadas, e verificar a curva calibrada; comparar com termómetro rastreável. Apagar pontos, confirmar regresso à curva Beta.
2. Definir `ODL_WEB_ADMIN_KEY` personalizado no firmware, configurar nova rede pela Web, confirmar que a palavra-passe não é devolvida por `/api/status`, reiniciar e confirmar a ligação. Não ensaiar na rede de produção sem autorização.
3. Testar duas redes guardadas e o AP de recurso; confirmar que o IP muda e é apresentado no Serial Monitor.

## D. Com microSD ligado (opcional)

1. Confirmar alimentação e adaptação 3,3 V/5 V do módulo e a pinagem SPI. Alterar `ODL_ENABLE_SD` para 1 e carregar firmware.
2. Em Sessões, verificar `Cartão ligado`, iniciar gravação, adquirir 10 minutos com simulação, parar e descarregar `ODLxxxx.CSV`.
3. Importar o CSV no Safari e verificar amostras e flancos, incluindo tempos e estados. Confirmar que a lista de canais corresponde aos metadados C do ficheiro.
4. Repetir com 100 ms e tráfego Web intenso: FIFO sem perdas, contadores `errors` e `missedEdges` a zero. Testar recuperação de alimentação e ficheiro truncado. Não retirar cartão durante escrita.

## Critérios antes de chamar à 1.0.1 estável

- Verify/Upload sem erros no núcleo oficial e tamanho de memória disponível publicado.
- FIFO/períodos perdidos nulos nos cenários-alvo (ou limites documentados); prioridade das ISR demonstrada.
- Temporização digital testada com gerador de impulsos, não apenas com simulação.
- Wi-Fi após reinício, armazenamento EEPROM e SD real testados sem corrupção.
- Revisão de segurança HTTP local e separação eléctrica dos 230 V.

## E. Sem microSD conectado

1. Carregar o firmware com `ODL_ENABLE_SD 1` e nenhum módulo físico.
2. Confirmar que o Arduino arranca, serve a página e mantém a aquisição regular.
3. Verificar em Sessões: `ready=0` e sem gravação.
4. Repetir após ligar um módulo compatível, com a placa desligada: confirmar `ready=1`, `logging=1` e novo ficheiro CSV; desligar com segurança antes de remover o cartão.

## F. Compatibilidade dos ficheiros de segurança

1. Testar a compilação com `arduino_secrets.h` antigo sem `ODL_WEB_ADMIN_KEY`.
2. Confirmar que a aquisição funciona mas a alteração de redes pela Web devolve recusa de autorização.
3. Adicionar a macro pessoal e repetir o teste Web apenas em rede de confiança.

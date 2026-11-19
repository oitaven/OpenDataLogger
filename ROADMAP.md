# OpenDataLogger — roteiro após 1.0.1 experimental

**Data:** 19-11-2026. A versão 1.0.0 é uma integração de funcionalidades de protótipo e **não é uma declaração de fiabilidade metrológica ou industrial**.

| Área | Estado 1.0.1 | Evolução ou verificação pendente |
|---|---|---|
| Aquisição comum | Implementada experimentalmente: FspTimer, FIFO 64, ADC A0–A5 | Testar no RA4M1; medir jitter, bloqueios ADC e perdas sob carga máxima |
| Eventos digitais | Implementados: D2/D3 por interrupção e histórico separado de 128 flancos | Testar impulsos físicos, limites de débito, `micros()` e overflow |
| Gráficos | Cinco modos, eixo temporal comum, matriz 12×8 (1/3/10/30 s), cursor duplo | Melhorar ampliação/zoom, acessibilidade e estabilidade temporal |
| Histórico curto | 4 KB RAM para amostras; 128 eventos; ODLB v2 incremental | Avaliar orçamento de SRAM real, perdas em desconexões e reconexão |
| Histórico longo sem SD | Sessão no Safari, exportação JSON/CSV, importação, replay e comparação | Testes com sessões extensas e limites do navegador; persistência automática opcional |
| microSD | Código SPI compilado, detecção no arranque, gravação automática quando há cartão, CSV, lista e descarga | Ligar módulo compatível com 3,3 V, medir bloqueios, perdas e resistência a corte de energia |
| Configuração | Canais, nomes, unidades/escala e EEPROM | Perfis de experiências e cópias de segurança completos |
| Canais calculados | Quatro canais virtuais no navegador | Integrais, potência com sensores adequados, expressões e exportação de séries calculadas |
| Alarmes | Quatro regras no firmware; flancos digitais também alimentam alarmes | Ensaios de duração, composição de regras, reconexão e eventos perdidos |
| Calibração | Até três pontos NTC, regressão Steinhart–Hart, EEPROM | Validar contra sensores conhecidos e tabelas Liebherr específicas |
| Relatórios | Contagem e duração de ciclos por canal digital e extremos por canal analógico | Estatísticas térmicas avançadas, relatório PDF e correlação causal |
| Wi-Fi | Alteração de duas redes e AP na Web, com código e activação após reinício | Auditoria de segurança: HTTP sem TLS, restante API não autenticada; modo de provisionamento físico |
| Relógio | Âncora UTC via rede/navegador e tempo monotónico | RTC e sincronização persistente através de falhas de energia |
| Expansão | Estrutura de canais genérica | ADCs externos I²C/SPI, módulos isolados AC e ampliação de entradas |
| UNO Q | Não implementado | Portar a aquisição para microcontrolador e Web/Linux para processador, com testes separados |
| Publicação | Código e documentação estruturados | Escolher licença, automatizar testes, reproduzir builds e publicar após revisão de segredos |

## Prioridade de ensaios na placa

1. **Não perder amostras:** verificar FIFO, períodos IRQ saltados, tempo de execução ADC e discrepâncias de flancos em carga Wi-Fi intensa.
2. **Tempos relativos correctos:** comparar impulsos digitais reais de 1 ms, 10 ms, 250 ms, com oscilloscope/gerador e duas janelas temporais.
3. **Alarmes e triggers fiáveis:** validar regras digitais curtas e analógicas independentemente da interface Web.
4. **Credenciais e EEPROM:** alterar rede sem recompilar, reiniciar, reentrar, confirmar integridade e modos de recuperação.
5. **microSD:** ensaiar na placa **após** ligar módulo com níveis lógicos adequados; desligar alimentação durante gravação e avaliar corrupção.

## Princípios preservados

- Uma frequência comum para os canais analógicos para permitir correlação temporal; flancos digitais por interrupção, sem depender desse intervalo.
- Não fazer operações de rede, escrita SD, cálculo de relatórios ou configuração demorada dentro das ISR.
- Nunca conectar tensão de rede de 230 V aos pinos: exigir interface isolada e segura, validada antes dos ensaios do frigorífico.
- Sem base de dados no Arduino; ficheiros e buffers simples. Separar aquisição, armazenamento, transporte e visualização.

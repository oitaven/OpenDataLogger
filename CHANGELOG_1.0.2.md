# OpenDataLogger v1.0.2 — Usabilidade e configuração

- `ChannelDefaults.h`: perfil inicial dos oito canais, editável e genérico; **não substitui** configurações válidas da EEPROM.
- Arranque em AQUISIÇÃO REAL; botões visíveis para escolher DEMO/AQUISIÇÃO REAL.
- DEMO: A0 triangular, A1 sinusoidal com tabela de 64 pontos/interpolação (não usa `sin()` na ISR), D2 quadrada, D3 impulsos.
- Indicador de sessão no navegador (inactiva/a gravar/parada) e botões destacados.
- Tabela de análise mais compacta, com valores próximos das etiquetas.
- Formulário Wi-Fi responsivo.
- Instruções NTC directamente na interface; impede guardar ponto quando o modo DEMO está activo.
- Instruções de FAT32 no separador Sessões. Sem botão de formatação remota (risco de perda de dados).
- Textos distintos para aquisição contínua e análise de períodos já guardados.

## Segurança e limites
A aquisição temporizada GPT e a FIFO não foram redesenhadas. O código foi inspeccionado e a interface verificada estaticamente, mas **o firmware completo ainda requer compilação no Arduino IDE com Renesas e ensaio físico**.

### Alterações do modo de aquisição
Mudar de DEMO para REAL repõe o histórico RAM e a referência do simulador; eventos e sessões exportados não são afectados.

### Memória EEPROM
A estrutura e a versão das definições guardadas permanecem as mesmas. A edição de `ChannelDefaults.h` só é usada quando não há definições EEPROM válidas. Para mudar os canais existentes, usa a página Web e guarda a configuração.

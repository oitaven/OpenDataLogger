# OpenDataLogger 1.0.1 — 19 de Novembro de 2026

- Corrigida dependência de `WEB_ADMIN_KEY` no ficheiro de credenciais legado. Agora `ODL_WEB_ADMIN_KEY` é uma macro opcional com valor de reserva que desactiva alterações de rede, sem quebrar a compilação.
- Nomes genéricos de fábrica para A0–A5 e D2/D3; botão para repor apenas nomes, sem limpar EEPROM de configurações existentes.
- Relatórios de ciclos por qualquer entrada digital, sem compressor ou válvula predefinidos; máximos e mínimos por entrada analógica.
- Sessões com título genérico e comparação temporal pelo primeiro flanco digital de subida (ou início).
- Suporte microSD compilado de fábrica: detecção no arranque e gravação automática quando o cartão está disponível. Sem cartão continua a operar em RAM/Safari.
- Mantidos o temporizador GPT prioritário, FIFO, Wi-Fi binário e gráficos da série anterior.

**Validação:** alterações de código e JavaScript revistas estaticamente. Não compilado no ambiente Arduino Renesas nem testado com o módulo SD na placa.

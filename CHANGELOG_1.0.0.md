# OpenDataLogger 1.0.0 (experimental) — 19-11-2026

Mantidas as implementações de aquisição GPT + FIFO, ring buffer, flancos digitais, `/api/bin` ODLB v2, matriz sincronizada e interface optimizada da 0.4.7.

## Acrescentado

- `AdvancedFeatures.h`: 4 regras de alarme, eventos de alarme (anel 32), 4 canais calculados configuráveis, até 3 pontos por NTC (Steinhart–Hart com 3 pontos), EEPROM com verificação de integridade, rede persistente com duas redes e AP.
- `OptionalSD.h`: compilação condicional com `ODL_ENABLE_SD`, gravação CSV bruta fora das ISR, lista/transferência de sessões, contadores de erros, indicação de flancos perdidos.
- Interface Web: separadores Análise e Sessões, dois cursores, Δt/Δvalores, cálculo de ciclos, exportação de relatório, captura de sessão no Safari, exportação JSON/CSV, importação e reprodução, comparação de experiências, configuração Wi-Fi em formulário POST.
- Aquisição GPT inicia antes da pesquisa Wi-Fi, permitindo captar durante o arranque da ligação (sujeito a capacidade da FIFO).
- Configuração de metadados e alarmes passa a não desligar o GPT para a escrita EEPROM. Canal de tipo diferente continua a exigir reinício do histórico.
- Corrigida a formatação `displayClock()` quando não existe hora sincronizada, e restaurado o painel de diagnósticos na UI.

## Testes internos

- Sintaxe/funcionalidade C++ com simuladores Arduino e compilação da variante SD activada com interfaces simuladas.
- Estrutura JSON de `/api/status`, alarmes e calibração SH com três referências.
- JavaScript e Chromium: cinco modos de gráficos, cursores, configuração, alarmes, análise, captura/importação/reprodução de sessões, comparação, Wi-Fi.

## Por validar

Compilação no núcleo Renesas 1.6.0, footprint exacto SRAM/flash, fiabilidade das interrupções ADC/GPIO, rede WiFiS3, EEPROM e SD reais, perda de amostras com tráfego intenso, credenciais/rede em hardware e precisão metrológica. Não considerar esta versão finalizada para aplicações industriais ou segurança de processos.

- Alarmes sobre D2/D3 também consomem o registo de flancos fora da interrupção, preservando a detecção de impulsos breves mesmo quando a amostragem ADC é lenta; as lacunas de eventos são contabilizadas.

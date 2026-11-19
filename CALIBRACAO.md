# Calibração e interpretação das leituras

O OpenDataLogger é um instrumento de diagnóstico genérico; a calibração depende **do tipo de grandeza** selecionado para cada entrada. Não é uma certificação metrológica.

## Entradas de tensão, corrente, potência e personalizadas

O valor exibido é calculado pela conversão ADC, seguida dos parâmetros **ganho** e **offset**, ambos definidos em `Canais`. A calibração é manual nesta versão: medir uma entrada com instrumento de referência e ajustar o ganho e o desvio. Calibração multiponto genérica de tensão/corrente ainda não está implementada.

## Entradas NTC (termístor)

A sonda NTC é um resistor cuja resistência diminui com a temperatura. O OpenDataLogger assume um divisor resistivo de 12 bits e usa `R fixa`, `R25` e `Beta` para converter ADC em resistência e temperatura. O circuito físico deve corresponder ao modelo utilizado. A sonda deve estar estabilizada na temperatura de referência.

- **Calibração rápida em `Canais`:** inserir a temperatura conhecida e usar `Calibrar NTC` (corrige um desvio fixo). Nesta operação, o firmware faz uma leitura directa e reinicia a aquisição/histórico para não misturar grandezas reinterpretadas.
- **1 ponto em `Análise`:** guarda o par ADC/temperatura e corrige um desvio da curva Beta.
- **2 pontos:** interpola a correcção da curva Beta entre os dois pontos, limitando extrapolações.
- **3 pontos:** calcula os coeficientes A, B e C de **Steinhart–Hart** e usa a equação `1/T = A + B·ln(R) + C·(ln(R))³` (T em kelvin, R em ohms).

Para acrescentar pontos, estabilizar o sensor a uma temperatura conhecida, seleccionar `Análise → Calibração NTC multiponto`, introduzir a temperatura medida por um termómetro de referência e escolher `Guardar ponto da leitura actual`. Repetir em três temperaturas distintas, distribuídas pelo intervalo de trabalho. Clicar depois em `Guardar configuração`, que escreve os pontos na EEPROM.

Se os valores `R fixa`, `R25`, `Beta` ou a montagem do divisor estiverem errados, a curva resultante também estará. Evitar autoaquecimento da sonda, contactos instáveis e leituras em temperaturas ainda não estabilizadas. Uma curva que passa por três pontos não garante precisão entre esses pontos, pelo que convém testar temperaturas intermédias.

## Armazenamento e integridade

Os dados brutos do ADC são mantidos nas sessões, pelo que é possível recalcular temperaturas no navegador. As referências e configuração deverão acompanhar cada sessão; não reprocessar uma experiência histórica com uma calibração diferente sem o indicar.


**Versão 1.0.2:** não guardar pontos NTC no modo DEMO; desligar DEMO antes de medir. A função «Calibrar NTC» em Canais aplica um ajuste de desvio; a multiponto em Análise guarda 1–3 pares de ADC/temperatura real.

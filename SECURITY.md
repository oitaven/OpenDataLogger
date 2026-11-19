# Segurança e limitações

O OpenDataLogger **é um protótipo experimental**, não um instrumento de segurança funcional nem um produto de medição certificado.

- **Não ligar 230 V directamente** ao Arduino. Utilizar interfaces adequadamente isoladas e validadas por profissional competente.
- **Nunca publicar** `arduino_secrets.h`: contém SSIDs, palavras-passe e o código de administração. Apenas `arduino_secrets.h.example` deve ser versionado.
- A interface HTTP **não usa TLS**. O código de administração protege as alterações de Wi-Fi, mas **outros endpoints de configuração não têm autenticação**. Utilizar apenas em redes locais de confiança; nunca encaminhar a porta HTTP para a Internet.
- Credenciais EEPROM não são cifradas contra acesso físico; proteger o dispositivo e a rede.
- Gravação microSD e reacção a falhas de alimentação exigem ensaios adicionais. A versão actual pode perder dados se houver corte de energia, FIFO cheia ou desconexão prolongada.
- A aquisição ADC por interrupção de temporizador tem de ser validada na placa física para as frequências pretendidas.
- Qualquer regra/alarme é informativo: **não o usar como mecanismo exclusivo de protecção**.

Para reportar falhas de segurança, evitar publicar passwords, IPs privados, capturas com dados sensíveis ou chaves em issues públicos. Preferir um reporte privado ao responsável do repositório.

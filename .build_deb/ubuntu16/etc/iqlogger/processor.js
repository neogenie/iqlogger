(function (message) {

    const journalData = message.data;
    const journalTimestamp = Math.round(message.timestamp / 1000) / 1000;

    return result = {
        "version": "1.1",
        "host": journalData._HOSTNAME,
        "timestamp": journalTimestamp,
        "level": journalData.PRIORITY,
        "short_message": journalData.MESSAGE,
        "_unit": journalData._SYSTEMD_UNIT,
        "_service": journalData.SYSLOG_IDENTIFIER
    };
})

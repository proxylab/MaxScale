CREATE TABLE `queries` (
    `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT,
    `clientName` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
    `serverId` bigint(20) unsigned NOT NULL,
    `transactionId` varchar(50) COLLATE utf8_unicode_ci DEFAULT NULL,
    `duration` double(24,3) unsigned NOT NULL,
    `requestTime` datetime NOT NULL,
    `responseTime` datetime NOT NULL,
    `statementType` int(11) NOT NULL,
    `canonCommandType` TINYINT(3) NOT NULL,
    `sqlQuery` varchar(2048) COLLATE utf8_unicode_ci NOT NULL,
    `canonicalSqlHash` bigint(20) unsigned DEFAULT NULL,
    `affectedTables` varchar(256) COLLATE utf8_unicode_ci DEFAULT NULL,
    `serverName` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
    `serverUniqueName` varchar(50) COLLATE utf8_unicode_ci NOT NULL,
    `isRealQuery` tinyint(4) NOT NULL DEFAULT '0',
    `queryFailed` TINYINT NOT NULL DEFAULT 0,  
    `queryError` VARCHAR(512),
    `createdAt` timestamp NOT NULL DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (`id`),
    KEY `canon_indx` (`canonicalSqlHash`) USING BTREE
) ENGINE=myISAM DEFAULT CHARSET=utf8 COLLATE=utf8_unicode_ci;
#include "databasemanager.h"
#include <QThread>

QThreadStorage<QSqlDatabase> DatabaseManager::m_threadLocalConnections;
QMutex DatabaseManager::m_threadLocalMutex;

// ================================================================
//  Singleton
// ================================================================
DatabaseManager& DatabaseManager::instance()
{
    static DatabaseManager s_instance;
    return s_instance;
}

DatabaseManager::DatabaseManager(const QString& connectionName)
    : m_isOpen(false)
{
    Q_UNUSED(connectionName);
}

DatabaseManager::DatabaseManager()
    : DatabaseManager(QString())
{ }

DatabaseManager::~DatabaseManager() { close(); }

// ================================================================
//  Identifier helpers
// ================================================================
QString DatabaseManager::safeIdent(const QString& ident)
{
    if (ident.isEmpty() || ident == "*") return ident;

    QStringList parts = ident.split('.');
    for (QString& p : parts) {
        if (p.isEmpty() || p == "*") continue;
        if (p.startsWith('"') && p.endsWith('"')) continue;

        bool needsQuote = false;
        for (QChar c : p) {
            if (!c.isLetterOrNumber() && c != '_') { needsQuote = true; break; }
        }
        if (needsQuote) {
            QString q = p;
            q.replace('"', "\"\"");
            p = '"' + q + '"';
        }
    }
    return parts.join('.');
}

QStringList DatabaseManager::safeIdentList(const QStringList& idents)
{
    QStringList out;
    for (const QString& s : idents) out.append(safeIdent(s));
    return out;
}

// ================================================================
//  Connection management
// ================================================================
bool DatabaseManager::open(const QString& host, int port,
                           const QString& dbName,
                           const QString& user,
                           const QString& password,
                           const QString& connectionName,
                           int connectTimeout)
{
    QMutexLocker locker(&m_mutex);

    if (m_isOpen) { qDebug() << "Database already open."; return true; }

    QString connName = connectionName.isEmpty()
                           ? QString("psql_%1").arg(quintptr(this), 0, 16)
                           : connectionName;

    m_db = QSqlDatabase::addDatabase("QPSQL", connName);
    m_db.setHostName(host);
    m_db.setPort(port);
    m_db.setDatabaseName(dbName);
    m_db.setUserName(user);
    m_db.setPassword(password);

    if (connectTimeout > 0)
        m_db.setConnectOptions(QString("connect_timeout=%1").arg(connectTimeout));

    if (!m_db.open()) {
        m_lastError = m_db.lastError().text();
        qDebug() << "PostgreSQL connection failed:" << m_lastError;
        m_isOpen = false;
        return false;
    }

    m_isOpen = true;
    m_lastError.clear();
    qDebug() << "PostgreSQL connected to" << dbName << "on" << host << ":" << port;
    return true;
}

void DatabaseManager::close()
{
    QMutexLocker locker(&m_mutex);
    if (m_isOpen) {
        m_db.close();
        m_isOpen = false;
        QString connName = m_db.connectionName();
        m_db = QSqlDatabase();
        QSqlDatabase::removeDatabase(connName);
        qDebug() << "PostgreSQL connection closed.";
    }
}

bool DatabaseManager::isOpen() const
{
    QMutexLocker locker(&m_mutex);
    return m_isOpen && m_db.isOpen();
}

QString DatabaseManager::lastError() const
{
    QMutexLocker locker(&m_mutex);
    return m_lastError;
}

QSqlDatabase DatabaseManager::database() const
{
    QMutexLocker locker(&m_mutex);
    return m_db;
}

// ================================================================
//  Low-level query execution
// ================================================================
QSqlQuery DatabaseManager::execQuery(const QString& sql)
{
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) { m_lastError = "Database not open"; return QSqlQuery(); }

    QSqlQuery query(m_db);
    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        qDebug() << "Query failed:" << m_lastError << "\nSQL:" << sql;
    } else {
        m_lastError.clear();
    }
    return query;
}

bool DatabaseManager::execNonQuery(const QString& sql)
{
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) { m_lastError = "Database not open"; return false; }

    QSqlQuery query(m_db);
    if (!query.exec(sql)) {
        m_lastError = query.lastError().text();
        qDebug() << "Non-query failed:" << m_lastError << "\nSQL:" << sql;
        return false;
    }
    m_lastError.clear();
    return true;
}

// ---- unlocked internals ----
QSqlQuery DatabaseManager::execPreparedQueryInternal(const QString& sql,
                                                     const QVariantList& params)
{
    QSqlQuery query(m_db);
    if (!query.prepare(sql)) {
        m_lastError = query.lastError().text();
        qDebug() << "Prepare failed:" << m_lastError << "\nSQL:" << sql;
        return QSqlQuery();
    }
    for (int i = 0; i < params.size(); ++i)
        query.bindValue(i, params.at(i));

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qDebug() << "Prepared query execution failed:" << m_lastError
                 << "\nSQL:" << sql;
        return QSqlQuery();
    }
    m_lastError.clear();
    return query;
}

bool DatabaseManager::execPreparedNonQueryInternal(const QString& sql,
                                                   const QVariantList& params)
{
    QSqlQuery query(m_db);
    if (!query.prepare(sql)) {
        m_lastError = query.lastError().text();
        qDebug() << "Prepare failed:" << m_lastError << "\nSQL:" << sql;
        return false;
    }
    for (int i = 0; i < params.size(); ++i)
        query.bindValue(i, params.at(i));

    if (!query.exec()) {
        m_lastError = query.lastError().text();
        qDebug() << "Prepared non-query failed:" << m_lastError << "\nSQL:" << sql;
        return false;
    }
    m_lastError.clear();
    return true;
}

// ---- public wrappers ----
QSqlQuery DatabaseManager::execPreparedQuery(const QString& sql,
                                             const QVariantList& params)
{
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) { m_lastError = "Database not open"; return QSqlQuery(); }
    return execPreparedQueryInternal(sql, params);
}

bool DatabaseManager::execPreparedNonQuery(const QString& sql,
                                           const QVariantList& params)
{
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) { m_lastError = "Database not open"; return false; }
    return execPreparedNonQueryInternal(sql, params);
}

// ================================================================
//  Table management
// ================================================================
bool DatabaseManager::createTable(const QString& tableName,
                                  const QList<ColumnDef>& columns,
                                  bool ifNotExists)
{
    if (tableName.isEmpty()) { m_lastError = "Table name cannot be empty"; return false; }
    if (columns.isEmpty())   { m_lastError = "Column definitions cannot be empty"; return false; }

    QString sql = "CREATE TABLE ";
    if (ifNotExists) sql += "IF NOT EXISTS ";
    sql += safeIdent(tableName) + " (";

    QStringList defs;
    for (const ColumnDef& col : columns) {
        QString def = safeIdent(col.name) + " " + col.type;
        if (!col.constraints.isEmpty()) def += " " + col.constraints;
        defs.append(def);
    }
    sql += defs.join(", ") + ")";
    return execNonQuery(sql);
}

bool DatabaseManager::tableExists(const QString& tableName)
{
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) { m_lastError = "Database not open"; return false; }

    QSqlQuery q = execPreparedQueryInternal(
        "SELECT EXISTS (SELECT 1 FROM information_schema.tables "
        "WHERE table_schema = current_schema() AND table_name = ?)",
        { tableName });
    return q.next() && q.value(0).toBool();
}

bool DatabaseManager::dropTable(const QString& tableName, bool ifExists)
{
    return execNonQuery(QString("DROP TABLE %1%2")
                            .arg(ifExists ? "IF EXISTS " : "", safeIdent(tableName)));
}

bool DatabaseManager::truncateTable(const QString& tableName)
{
    return execNonQuery("TRUNCATE TABLE " + safeIdent(tableName) +
                        " RESTART IDENTITY CASCADE");
}

TableBuilder DatabaseManager::createTableBuilder(const QString& tableName)
{
    return TableBuilder(*this, tableName);
}

// ================================================================
//  Insert
// ================================================================
bool DatabaseManager::insert(const QString& tableName, const QVariantMap& values)
{
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) { m_lastError = "Database not open"; return false; }
    if (tableName.isEmpty()) { m_lastError = "Table name cannot be empty"; return false; }
    if (values.isEmpty())    { m_lastError = "No values provided for insert"; return false; }

    QStringList cols, phs;
    QVariantList params;
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        cols.append(it.key());
        phs.append("?");
        params.append(it.value());
    }

    const QString sql = QString("INSERT INTO %1 (%2) VALUES (%3)")
                            .arg(safeIdent(tableName),
                                 safeIdentList(cols).join(", "),
                                 phs.join(", "));

    return execPreparedNonQueryInternal(sql, params);
}

QVariant DatabaseManager::insertReturningId(const QString& tableName,
                                            const QVariantMap& values,
                                            const QString& idColumn)
{
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) { m_lastError = "Database not open"; return QVariant(); }
    if (values.isEmpty()) { m_lastError = "No values provided"; return QVariant(); }

    QStringList cols, phs;
    QVariantList params;
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        cols.append(it.key());
        phs.append("?");
        params.append(it.value());
    }

    const QString sql = QString("INSERT INTO %1 (%2) VALUES (%3) RETURNING %4")
                            .arg(safeIdent(tableName),
                                 safeIdentList(cols).join(", "),
                                 phs.join(", "),
                                 safeIdent(idColumn));

    QSqlQuery q = execPreparedQueryInternal(sql, params);
    if (!q.next()) return QVariant();
    return q.value(0);
}

bool DatabaseManager::insertMany(const QString& tableName,
                                 const QList<QVariantMap>& rows)
{
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen)     { m_lastError = "Database not open"; return false; }
    if (rows.isEmpty()) return true;

    // Union of all column names (preserving order of first occurrence)
    QStringList cols;
    for (const QVariantMap& row : rows) {
        for (auto it = row.constBegin(); it != row.constEnd(); ++it) {
            if (!cols.contains(it.key())) cols.append(it.key());
        }
    }
    if (cols.isEmpty()) { m_lastError = "No columns in any row"; return false; }

    QStringList phs;
    for (int i = 0; i < cols.size(); ++i) phs.append("?");

    const QString sql = QString("INSERT INTO %1 (%2) VALUES (%3)")
                            .arg(safeIdent(tableName),
                                 safeIdentList(cols).join(", "),
                                 phs.join(", "));

    // Try to own a transaction; if we're already in one, just participate in it.
    const bool ownTx = m_db.transaction();

    QSqlQuery q(m_db);
    if (!q.prepare(sql)) {
        m_lastError = q.lastError().text();
        if (ownTx) m_db.rollback();
        return false;
    }

    for (const QVariantMap& row : rows) {
        for (int i = 0; i < cols.size(); ++i) {
            q.bindValue(i, row.contains(cols.at(i))
                        ? row.value(cols.at(i))
                        : QVariant());
        }
        if (!q.exec()) {
            m_lastError = q.lastError().text();
            if (ownTx) m_db.rollback();
            return false;
        }
    }

    if (ownTx && !m_db.commit()) {
        m_lastError = m_db.lastError().text();
        m_db.rollback();
        return false;
    }
    m_lastError.clear();
    return true;
}

bool DatabaseManager::upsert(const QString& tableName, const QVariantMap& values,
                             const QStringList& conflictColumns)
{
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen)     { m_lastError = "Database not open"; return false; }
    if (values.isEmpty()) { m_lastError = "No values provided"; return false; }

    QStringList cols, phs;
    QVariantList params;
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        cols.append(it.key());
        phs.append("?");
        params.append(it.value());
    }

    QStringList updates;
    for (const QString& c : cols) {
        if (!conflictColumns.contains(c))
            updates.append(QString("%1 = EXCLUDED.%1").arg(safeIdent(c)));
    }

    QString sql = QString("INSERT INTO %1 (%2) VALUES (%3) ON CONFLICT (%4)")
                      .arg(safeIdent(tableName),
                           safeIdentList(cols).join(", "),
                           phs.join(", "),
                           safeIdentList(conflictColumns).join(", "));
    sql += updates.isEmpty() ? " DO NOTHING"
                             : " DO UPDATE SET " + updates.join(", ");

    return execPreparedNonQueryInternal(sql, params);
}

// ================================================================
//  Update / Delete
// ================================================================
bool DatabaseManager::update(const QString& tableName, const QVariantMap& values,
                             const QString& whereClause,
                             const QVariantList& whereParams)
{
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen)        { m_lastError = "Database not open"; return false; }
    if (values.isEmpty()) { m_lastError = "No values provided"; return false; }

    QStringList setParts;
    QVariantList params;
    for (auto it = values.constBegin(); it != values.constEnd(); ++it) {
        setParts.append(safeIdent(it.key()) + " = ?");
        params.append(it.value());
    }

    QString sql = "UPDATE " + safeIdent(tableName) +
                  " SET " + setParts.join(", ");
    if (!whereClause.trimmed().isEmpty()) {
        sql += " WHERE " + whereClause;
        params.append(whereParams);
    }

    return execPreparedNonQueryInternal(sql, params);
}

int DatabaseManager::deleteRows(const QString& tableName,
                                const QString& whereClause,
                                const QVariantList& whereParams)
{
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) { m_lastError = "Database not open"; return -1; }

    QString sql = "DELETE FROM " + safeIdent(tableName);
    if (!whereClause.trimmed().isEmpty())
        sql += " WHERE " + whereClause;

    QSqlQuery q(m_db);
    if (!q.prepare(sql)) { m_lastError = q.lastError().text(); return -1; }
    for (int i = 0; i < whereParams.size(); ++i) q.bindValue(i, whereParams.at(i));
    if (!q.exec()) { m_lastError = q.lastError().text(); return -1; }

    const int n = q.numRowsAffected();
    m_lastError.clear();
    return n;
}

// ================================================================
//  Select
// ================================================================
QList<QVariantMap> DatabaseManager::selectWhereInternal(
    const QString& tableName, const QStringList& columns,
    const QString& whereClause, const QVariantList& whereParams,
    const QString& orderBy, int limit, int offset)
{
    QList<QVariantMap> results;

    const QString colsSql = columns.isEmpty()
                                ? "*"
                                : safeIdentList(columns).join(", ");

    QString sql = QString("SELECT %1 FROM %2").arg(colsSql, safeIdent(tableName));
    if (!whereClause.trimmed().isEmpty()) sql += " WHERE " + whereClause;
    if (!orderBy.trimmed().isEmpty())     sql += " ORDER BY " + orderBy;
    if (limit > 0)                        sql += QString(" LIMIT %1").arg(limit);
    if (offset > 0)                       sql += QString(" OFFSET %1").arg(offset);

    QSqlQuery q = execPreparedQueryInternal(sql, whereParams);
    const QSqlRecord rec = q.record();
    while (q.next()) {
        QVariantMap row;
        for (int i = 0; i < rec.count(); ++i)
            row.insert(rec.fieldName(i), q.value(i));
        results.append(row);
    }
    return results;
}

QList<QVariantMap> DatabaseManager::selectWhere(
    const QString& tableName, const QStringList& columns,
    const QString& whereClause, const QVariantList& whereParams,
    const QString& orderBy, int limit, int offset)
{
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) { m_lastError = "Database not open"; return {}; }
    return selectWhereInternal(tableName, columns, whereClause, whereParams,
                               orderBy, limit, offset);
}

QList<QVariantMap> DatabaseManager::selectAll(const QString& tableName)
{
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) { m_lastError = "Database not open"; return {}; }
    return selectWhereInternal(tableName, {}, {}, {}, {}, -1, -1);
}

QVariantMap DatabaseManager::findById(const QString& tableName,
                                      const QVariant& id,
                                      const QString& idColumn)
{
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) { m_lastError = "Database not open"; return {}; }

    const QList<QVariantMap> rows = selectWhereInternal(
        tableName, {}, safeIdent(idColumn) + " = ?", { id }, {}, 1, -1);
    return rows.isEmpty() ? QVariantMap() : rows.first();
}

qint64 DatabaseManager::count(const QString& tableName,
                              const QString& whereClause,
                              const QVariantList& whereParams)
{
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) { m_lastError = "Database not open"; return -1; }

    QString sql = "SELECT COUNT(*) FROM " + safeIdent(tableName);
    if (!whereClause.trimmed().isEmpty()) sql += " WHERE " + whereClause;

    QSqlQuery q = execPreparedQueryInternal(sql, whereParams);
    if (!q.next()) return -1;
    return q.value(0).toLongLong();
}

bool DatabaseManager::exists(const QString& tableName,
                             const QString& whereClause,
                             const QVariantList& whereParams)
{
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) { m_lastError = "Database not open"; return false; }

    QString sql = "SELECT EXISTS (SELECT 1 FROM " + safeIdent(tableName);
    if (!whereClause.trimmed().isEmpty()) sql += " WHERE " + whereClause;
    sql += ")";

    QSqlQuery q = execPreparedQueryInternal(sql, whereParams);
    return q.next() && q.value(0).toBool();
}

// ================================================================
//  Transactions
// ================================================================
bool DatabaseManager::beginTransaction()
{
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) { m_lastError = "Database not open"; return false; }
    if (!m_db.transaction()) {
        m_lastError = m_db.lastError().text();
        qDebug() << "Begin transaction failed:" << m_lastError;
        return false;
    }
    m_lastError.clear();
    return true;
}

bool DatabaseManager::commitTransaction()
{
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) { m_lastError = "Database not open"; return false; }
    if (!m_db.commit()) {
        m_lastError = m_db.lastError().text();
        qDebug() << "Commit failed:" << m_lastError;
        return false;
    }
    m_lastError.clear();
    return true;
}

bool DatabaseManager::rollbackTransaction()
{
    QMutexLocker locker(&m_mutex);
    if (!m_isOpen) { m_lastError = "Database not open"; return false; }
    if (!m_db.rollback()) {
        m_lastError = m_db.lastError().text();
        qDebug() << "Rollback failed:" << m_lastError;
        return false;
    }
    m_lastError.clear();
    return true;
}

// ================================================================
//  Thread-local connection
// ================================================================
QSqlDatabase DatabaseManager::threadConnection()
{
    QMutexLocker locker(&m_threadLocalMutex);

    if (!m_threadLocalConnections.hasLocalData()) {
        QString connName = QString("thread_%1")
        .arg(quintptr(QThread::currentThread()), 0, 16);
        QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", connName);
        m_threadLocalConnections.setLocalData(db);
    }
    return m_threadLocalConnections.localData();
}

QSqlDatabase DatabaseManager::threadConnection(const QString& host, int port,
                                               const QString& dbName,
                                               const QString& user,
                                               const QString& password,
                                               int connectTimeout)
{
    QMutexLocker locker(&m_threadLocalMutex);

    QString connName = QString("thread_%1")
                           .arg(quintptr(QThread::currentThread()), 0, 16);

    if (m_threadLocalConnections.hasLocalData()) {
        QSqlDatabase existing = m_threadLocalConnections.localData();
        if (existing.isOpen()) return existing;
        // Drop broken connection, recreate below.
        const QString oldName = existing.connectionName();
        existing = QSqlDatabase();
        QSqlDatabase::removeDatabase(oldName);
    }

    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL", connName);
    db.setHostName(host);
    db.setPort(port);
    db.setDatabaseName(dbName);
    db.setUserName(user);
    db.setPassword(password);
    if (connectTimeout > 0)
        db.setConnectOptions(QString("connect_timeout=%1").arg(connectTimeout));

    if (!db.open())
        qDebug() << "Thread-local connection failed:" << db.lastError().text();
    else
        qDebug() << "Thread-local connection opened for" << QThread::currentThread();

    m_threadLocalConnections.setLocalData(db);
    return db;
}

// ================================================================
//  TableBuilder
// ================================================================
TableBuilder::TableBuilder(DatabaseManager& db, const QString& tableName)
    : m_db(db), m_tableName(tableName)
{ }

TableBuilder& TableBuilder::id(const QString& name)
{
    m_columns.append({ name, "SERIAL", "PRIMARY KEY" });
    return *this;
}

TableBuilder& TableBuilder::serial(const QString& name)
{
    m_columns.append({ name, "SERIAL", "" });
    return *this;
}

TableBuilder& TableBuilder::integer(const QString& name, bool notNull)
{
    m_columns.append({ name, "INTEGER", notNull ? "NOT NULL" : "" });
    return *this;
}

TableBuilder& TableBuilder::bigint(const QString& name, bool notNull)
{
    m_columns.append({ name, "BIGINT", notNull ? "NOT NULL" : "" });
    return *this;
}

TableBuilder& TableBuilder::text(const QString& name, bool notNull)
{
    m_columns.append({ name, "TEXT", notNull ? "NOT NULL" : "" });
    return *this;
}

TableBuilder& TableBuilder::varchar(const QString& name, int length, bool notNull)
{
    m_columns.append({ name,
                      QString("VARCHAR(%1)").arg(length),
                      notNull ? "NOT NULL" : "" });
    return *this;
}

TableBuilder& TableBuilder::numeric(const QString& name, bool notNull)
{
    m_columns.append({ name, "NUMERIC", notNull ? "NOT NULL" : "" });
    return *this;
}

TableBuilder& TableBuilder::real(const QString& name, bool notNull)
{
    m_columns.append({ name, "REAL", notNull ? "NOT NULL" : "" });
    return *this;
}

TableBuilder& TableBuilder::boolean(const QString& name, bool notNull,
                                    bool defaultValue)
{
    QString constraints;
    if (notNull) constraints += "NOT NULL";
    if (defaultValue) constraints += " DEFAULT TRUE";
    m_columns.append({ name, "BOOLEAN", constraints.trimmed() });
    return *this;
}

TableBuilder& TableBuilder::timestamp(const QString& name, bool notNull)
{
    m_columns.append({ name, "TIMESTAMP", notNull ? "NOT NULL" : "" });
    return *this;
}

TableBuilder& TableBuilder::column(const ColumnDef& col)
{
    m_columns.append(col);
    return *this;
}

TableBuilder& TableBuilder::primaryKey(const QStringList& cols)
{
    m_tableConstraints.append(
        QString("PRIMARY KEY (%1)").arg(
            DatabaseManager::safeIdentList(cols).join(", ")));
    return *this;
}

TableBuilder& TableBuilder::unique(const QStringList& cols)
{
    m_tableConstraints.append(
        QString("UNIQUE (%1)").arg(
            DatabaseManager::safeIdentList(cols).join(", ")));
    return *this;
}

TableBuilder& TableBuilder::foreignKey(const QString& col,
                                       const QString& refTable,
                                       const QString& refCol)
{
    m_tableConstraints.append(
        QString("FOREIGN KEY (%1) REFERENCES %2 (%3)")
            .arg(DatabaseManager::safeIdent(col),
                 DatabaseManager::safeIdent(refTable),
                 DatabaseManager::safeIdent(refCol)));
    return *this;
}

TableBuilder& TableBuilder::ifNotExists(bool v)
{
    m_ifNotExists = v;
    return *this;
}

bool TableBuilder::create()
{
    if (m_tableName.isEmpty()) { m_lastError = "Table name cannot be empty"; return false; }
    if (m_columns.isEmpty())   { m_lastError = "At least one column required"; return false; }

    QStringList parts;
    for (const ColumnDef& col : m_columns) {
        QString def = DatabaseManager::safeIdent(col.name) + " " + col.type;
        if (!col.constraints.isEmpty()) def += " " + col.constraints;
        parts.append(def);
    }
    parts.append(m_tableConstraints);

    m_sql = QString("CREATE TABLE %1%2 (%3)")
                .arg(m_ifNotExists ? "IF NOT EXISTS " : "",
                     DatabaseManager::safeIdent(m_tableName),
                     parts.join(", "));

    if (!m_db.execNonQuery(m_sql)) {
        m_lastError = m_db.lastError();
        return false;
    }
    m_lastError.clear();
    return true;
}
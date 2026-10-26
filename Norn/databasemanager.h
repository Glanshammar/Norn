#ifndef DATABASEMANAGER_H
#define DATABASEMANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QSqlRecord>
#include <QDebug>
#include <QMutex>
#include <QThreadStorage>
#include <QThread>
#include <QVariantMap>
#include <QList>
#include <QStringList>

struct ColumnDef {
    QString name;
    QString type;
    QString constraints;
};

class TableBuilder;

class DatabaseManager
{
public:
    static DatabaseManager& instance();

    DatabaseManager(const QString& connectionName);
    ~DatabaseManager();

    DatabaseManager(const DatabaseManager&) = delete;
    DatabaseManager& operator=(const DatabaseManager&) = delete;

    // ---------------- Connection ----------------
    bool open(const QString& host, int port,
              const QString& dbName,
              const QString& user,
              const QString& password,
              const QString& connectionName = QString(),
              int connectTimeout = 10);
    void close();
    bool isOpen() const;
    QString lastError() const;
    QSqlDatabase database() const;

    // ---------------- Low-level queries ----------------
    QSqlQuery execQuery(const QString& sql);
    bool execNonQuery(const QString& sql);
    QSqlQuery execPreparedQuery(const QString& sql, const QVariantList& params);
    bool execPreparedNonQuery(const QString& sql, const QVariantList& params);

    // ---------------- Table management ----------------
    bool createTable(const QString& tableName,
                     const QList<ColumnDef>& columns,
                     bool ifNotExists = true);
    bool tableExists(const QString& tableName);
    bool dropTable(const QString& tableName, bool ifExists = true);
    bool truncateTable(const QString& tableName);

    // Fluent table creation: db.createTableBuilder("employees").id().text("name",true).create()
    TableBuilder createTableBuilder(const QString& tableName);

    // ---------------- Insert ----------------
    bool insert(const QString& tableName, const QVariantMap& values);
    // Returns the value of `idColumn` from the inserted row (via RETURNING), or an
    // invalid QVariant on error.
    QVariant insertReturningId(const QString& tableName, const QVariantMap& values,
                               const QString& idColumn = QStringLiteral("id"));
    // Batch insert; wraps in a transaction unless already inside one.
    bool insertMany(const QString& tableName, const QList<QVariantMap>& rows);
    // INSERT ... ON CONFLICT (cols) DO UPDATE SET ...
    bool upsert(const QString& tableName, const QVariantMap& values,
                const QStringList& conflictColumns);

    // ---------------- Update / Delete ----------------
    bool update(const QString& tableName, const QVariantMap& values,
                const QString& whereClause = QString(),
                const QVariantList& whereParams = QVariantList());
    // Returns number of deleted rows, or -1 on error.
    int deleteRows(const QString& tableName,
                   const QString& whereClause = QString(),
                   const QVariantList& whereParams = QVariantList());

    // ---------------- Select ----------------
    QList<QVariantMap> selectWhere(const QString& tableName,
                                   const QStringList& columns = QStringList(),
                                   const QString& whereClause = QString(),
                                   const QVariantList& whereParams = QVariantList(),
                                   const QString& orderBy = QString(),
                                   int limit = -1,
                                   int offset = -1);
    QList<QVariantMap> selectAll(const QString& tableName);
    QVariantMap findById(const QString& tableName, const QVariant& id,
                         const QString& idColumn = QStringLiteral("id"));
    // Returns -1 on error.
    qint64 count(const QString& tableName,
                 const QString& whereClause = QString(),
                 const QVariantList& whereParams = QVariantList());
    bool exists(const QString& tableName,
                const QString& whereClause = QString(),
                const QVariantList& whereParams = QVariantList());

    // ---------------- Transactions ----------------
    bool beginTransaction();
    bool commitTransaction();
    bool rollbackTransaction();

    // ---------------- Thread-local ----------------
    static QSqlDatabase threadConnection();  // Caller opens it
    static QSqlDatabase threadConnection(const QString& host, int port,
                                         const QString& dbName,
                                         const QString& user,
                                         const QString& password,
                                         int connectTimeout = 10);

private:
    DatabaseManager();

    friend class TableBuilder;

    QSqlDatabase m_db;
    bool m_isOpen;
    QString m_lastError;
    mutable QMutex m_mutex;

    static QThreadStorage<QSqlDatabase> m_threadLocalConnections;
    static QMutex m_threadLocalMutex;

    // --- Unlocked internals (caller MUST hold m_mutex) ---
    QSqlQuery execPreparedQueryInternal(const QString& sql, const QVariantList& params);
    bool execPreparedNonQueryInternal(const QString& sql, const QVariantList& params);
    QList<QVariantMap> selectWhereInternal(const QString& tableName,
                                           const QStringList& columns,
                                           const QString& whereClause,
                                           const QVariantList& whereParams,
                                           const QString& orderBy,
                                           int limit, int offset);

    // --- Identifier helpers ---
    static QString safeIdent(const QString& ident);
    static QStringList safeIdentList(const QStringList& idents);
};

// ---------------------------------------------------------------
// CREATE TABLE
// ---------------------------------------------------------------
class TableBuilder
{
public:
    explicit TableBuilder(DatabaseManager& db, const QString& tableName);

    TableBuilder& id(const QString& name = QStringLiteral("id"));  // SERIAL PRIMARY KEY
    TableBuilder& serial(const QString& name);
    TableBuilder& integer(const QString& name, bool notNull = false);
    TableBuilder& bigint(const QString& name, bool notNull = false);
    TableBuilder& text(const QString& name, bool notNull = false);
    TableBuilder& varchar(const QString& name, int length, bool notNull = false);
    TableBuilder& numeric(const QString& name, bool notNull = false);
    TableBuilder& real(const QString& name, bool notNull = false);
    TableBuilder& boolean(const QString& name, bool notNull = false,
                          bool defaultValue = false);
    TableBuilder& timestamp(const QString& name, bool notNull = false);
    TableBuilder& column(const ColumnDef& col);  // Escape hatch

    TableBuilder& primaryKey(const QStringList& cols);
    TableBuilder& unique(const QStringList& cols);
    TableBuilder& foreignKey(const QString& col,
                             const QString& refTable,
                             const QString& refCol = QStringLiteral("id"));

    TableBuilder& ifNotExists(bool v = true);

    bool create();
    QString lastError() const { return m_lastError; }
    QString sql() const { return m_sql; }

private:
    DatabaseManager& m_db;
    QString m_tableName;
    QList<ColumnDef> m_columns;
    QStringList m_tableConstraints;
    bool m_ifNotExists = true;
    QString m_lastError;
    QString m_sql;
};

#endif // DATABASEMANAGER_H
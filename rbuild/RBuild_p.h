#ifndef RBuild_p_h
#define RBuild_p_h

#include "AtomicString.h"
#include "GccArguments.h"
#include "RBuild.h"
#include <QList>
#include <QHash>
#include "RTags.h"
#include <Location.h>
#include <Database.h>

struct Entity {
    Entity() : kind(CXIdxEntity_Unexposed) {}
    QByteArray name;
    QList<QByteArray> parentNames;
    CXIdxEntityKind kind;
    Location definition;
    QSet<Location> declarations, references;
};

struct Source {
    Path path;
    QList<QByteArray> args;
    quint64 lastModified;
    QHash<Path, quint64> dependencies;
};

static inline QDebug operator<<(QDebug dbg, const Source &s)
{
    dbg << s.path << s.args << s.lastModified << s.dependencies;
    return dbg;
}

static inline QDataStream &operator<<(QDataStream &ds, const Source &s)
{
    ds << s.path << s.args << s.lastModified << s.dependencies;
    return ds;
}

static inline QDataStream &operator>>(QDataStream &ds, Source &s)
{
    ds >> s.path >> s.args >> s.lastModified >> s.dependencies;
    return ds;
}

struct RBuildPrivate
{
    RBuildPrivate()
        : db(0), pendingJobs(0), index(0)
    {
        Location::files() = &filesToIndex;
    }

    QHash<QByteArray, Entity> entities;
    QHash<Path, unsigned> filesToIndex;
    QHash<QByteArray, QList<Location> > references;
    Database *db;
    Path makefile, sourceDir, dbPath;
    MakefileParser parser;
    int pendingJobs;
    CXIndex index;
    QHash<Precompile*, QList<GccArguments> > filesByPrecompile;
    QList<GccArguments> files;
    QThreadPool threadPool;

    QList<Source> sources;
    QMutex entryMutex;

    inline int locationKey(const Location &loc, char buf[512]) const
    {
        if (!loc.file)
            return 0;
        const int ret = snprintf(buf, 512, "%d:%d:%d", loc.file, loc.line, loc.column);
        return ret;
    }
};

class Precompile;
class PrecompileRunnable : public QObject, public QRunnable
{
    Q_OBJECT
public:
    PrecompileRunnable(Precompile *pch,
                       RBuildPrivate *rbp,
                       CXIndex index) // ### is this threadsafe?
        : mPch(pch), mRBP(rbp), mIndex(index)
    {
        setAutoDelete(true);
    }
    virtual void run();
signals:
    void finished(Precompile *pre);
private:
    Precompile *mPch;
    RBuildPrivate *mRBP;
    CXIndex mIndex;
};

#endif

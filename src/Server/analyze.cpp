#include "analyze.h"
#include "network.h"
#include "player.h"
#include "../PokemonInfo/pokemonstructs.h"
#include "../PokemonInfo/battlestructs.h"

using namespace NetworkServ;

Analyzer::Analyzer(QTcpSocket *sock, int id) : mysocket(sock, id)
{
    connect(&socket(), SIGNAL(disconnected()), SIGNAL(disconnected()));
    connect(&socket(), SIGNAL(isFull(QByteArray)), this, SLOT(commandReceived(QByteArray)));
    connect(&socket(), SIGNAL(_error()), this, SLOT(error()));
    connect(this, SIGNAL(sendCommand(QByteArray)), &socket(), SLOT(send(QByteArray)));

    /* Only if its not registry */
    if (id != 0) {
        mytimer = new QTimer(this);
        connect(mytimer, SIGNAL(timeout()), this, SLOT(keepAlive()));
        mytimer->start(30000); //every 30 secs
    }
}

Analyzer::~Analyzer()
{
    blockSignals(true);
}

void Analyzer::sendMessage(const QString &message)
{
    notify(SendMessage, message);
}

void Analyzer::engageBattle(int id, const TeamBattle &team, const BattleConfiguration &conf)
{
    notify(EngageBattle, qint32(id), team, conf);
}

void Analyzer::connectTo(const QString &host, quint16 port)
{
    connect(&socket(), SIGNAL(connected()), SIGNAL(connected()));
    mysocket.connectToHost(host, port);
}

void Analyzer::close() {
    socket().close();
}

QString Analyzer::ip() const {
    return socket().ip();
}

void Analyzer::sendPlayer(int num, const BasicInfo &team, int auth)
{
    notify(PlayersList, qint32(num), team, qint8(auth));
}

void Analyzer::sendTeamChange(int num, const BasicInfo &team, int auth)
{
    notify(SendTeam, qint32(num), team, qint8(auth));
}

void Analyzer::sendPM(int dest, const QString &mess)
{
    notify(SendPM, qint32(dest), mess);
}

void Analyzer::sendLogin(int num, const BasicInfo &team, int auth)
{
    notify(Login, qint32(num), team, qint8(auth));
}

void Analyzer::sendLogout(int num)
{
    notify(Logout, qint32(num));
}

void Analyzer::keepAlive()
{
    notify(KeepAlive);
}

void Analyzer::sendChallengeStuff(const ChallengeInfo &c)
{
    notify(ChallengeStuff, c);
}

void Analyzer::sendBattleResult(quint8 res, int winner, int loser)
{
    notify(BattleFinished, res, qint32(winner), qint32(loser));
}

void Analyzer::sendBattleCommand(const QByteArray & command)
{
    notify(BattleMessage, command);
}

void Analyzer::sendUserInfo(const UserInfo &ui)
{
    notify(GetUserInfo, ui);
}

void Analyzer::error()
{
    emit connectionError(socket().error(), socket().errorString());
}

bool Analyzer::isConnected() const
{
    return socket().isConnected();
}


void Analyzer::commandReceived(const QByteArray &commandline)
{
    QDataStream in (commandline);
    in.setVersion(QDataStream::Qt_4_5);
    uchar command;

    in >> command;

    switch (command) {
    case Login:
	{
            if (mysocket.id() != 0) {
                TeamInfo team;
                in >> team;
                emit loggedIn(team);
            } else
                emit accepted(); // for registry;
	    break;
	}
    case SendMessage:
	{
	    QString mess;
	    in >> mess;
	    emit messageReceived(mess);
	    break;
	}
    case SendTeam:
	{
	    TeamInfo team;
	    in >> team;
	    emit teamReceived(team);
	    break;
	}
    case ChallengeStuff:
	{
            ChallengeInfo c;
            in >> c;
            emit challengeStuff(c);
	    break;
	}
    case BattleMessage:
	{
	    BattleChoice ch;
	    in >> ch;
	    emit battleMessage(ch);
	    break;
	}
    case BattleChat:
	{
	    QString s;
	    in >> s;
	    emit battleChat(s);
	    break;
	}
    case BattleFinished:
        emit forfeitBattle();
        break;
    case KeepAlive:
        break;
    case Register:
        if (mysocket.id() != 0)
            emit wannaRegister();
        else
            emit nameTaken(); // for registry
        break;
    case AskForPass:
        {
            QString hash;
            in >> hash;
            emit sentHash(hash);
            break;
        }
    case PlayerKick:
        {
            qint32 id;
            in >> id;
            emit kick(id);
            break;
        }
    case PlayerBan:
        {
            qint32 id;
            in >> id;
            emit ban(id);
            break;
        }
    case Logout:
        emit ipRefused();
        break;
    case ServNameChange:
        emit invalidName();
        break;
    case SendPM:
        {
            qint32 id;
            QString s;
            in >> id >> s;
            emit PMsent(id, s);
            break;
        }
    case GetUserInfo:
        {
            QString name;
            in >> name;
            emit getUserInfo(name);
            break;
        }
    case GetBanList:
        emit banListRequested();
        break;
    default:
        emit protocolError(UnknownCommand, tr("Protocol error: unknown command received"));
        break;
    }
}

Network & Analyzer::socket()
{
    return mysocket;
}

const Network & Analyzer::socket() const
{
    return mysocket;
}

void Analyzer::notify(int command)
{
    if (!isConnected())
        return;
    QByteArray tosend;
    QDataStream out(&tosend, QIODevice::WriteOnly);
    out.setVersion(QDataStream::Qt_4_5);

    out << uchar(command);

    emit sendCommand(tosend);
}

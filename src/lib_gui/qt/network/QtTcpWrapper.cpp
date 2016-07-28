#include "QtTcpWrapper.h"

#include "utility/logging/logging.h"

QtTcpWrapper::QtTcpWrapper(QObject* parent, const std::string& ip, const quint16 serverPort, const quint16 clientPort)
	: QObject(parent)
	, m_serverPort(serverPort)
	, m_clientPort(clientPort)
	, m_ip(ip)
{
}

void QtTcpWrapper::startListening()
{
	QHostAddress address(m_ip.c_str());

	m_tcpServer = new QTcpServer(this);

	connect(m_tcpServer, SIGNAL(newConnection()), this, SLOT(acceptConnection()));

	if (!m_tcpServer->listen(QHostAddress::LocalHost, m_serverPort))
	{
		LOG_ERROR_STREAM(<< "TCP server failed to start with error: \"" + m_tcpServer->errorString().toStdString() + "\". Unable to listen for IDE plugin messages.");
	}
}

QtTcpWrapper::~QtTcpWrapper()
{
	if (m_tcpServer != NULL)
	{
		if (m_tcpServer->isListening())
		{
			m_tcpServer->close();
		}

		delete m_tcpServer;
	}
}

void QtTcpWrapper::sendMessage(const std::string& message) const
{
	QByteArray data;
	data.append(message.c_str());

	QTcpSocket socket;
	socket.connectToHost(QHostAddress::LocalHost, m_clientPort);

	if (socket.waitForConnected())
	{
		socket.write(data);
		socket.flush();
		socket.waitForBytesWritten();
		socket.close();
	}
}

void QtTcpWrapper::setReadCallback(const std::function<void(const std::string&)>& callback)
{
	m_readCallback = callback;
}

quint16 QtTcpWrapper::getServerPort() const
{
	return m_serverPort;
}

void QtTcpWrapper::setServerPort(const quint16 serverPort)
{
	m_serverPort = serverPort;
}

quint16 QtTcpWrapper::getClientPort() const
{
	return m_clientPort;
}

void QtTcpWrapper::setClientPort(const quint16 clientPort)
{
	m_clientPort = clientPort;
}

bool QtTcpWrapper::isListening() const
{
	return m_tcpServer->isListening();
}

void QtTcpWrapper::acceptConnection()
{
	m_tcpClient = m_tcpServer->nextPendingConnection();

	connect(m_tcpClient, SIGNAL(readyRead()), this, SLOT(startRead()));
}

void QtTcpWrapper::startRead()
{
	char buffer[1024] = { 0 };
	m_tcpClient->read(buffer, m_tcpClient->bytesAvailable());
	m_tcpClient->close();

	if (m_readCallback != NULL)
	{
		m_readCallback(buffer);
	}
}

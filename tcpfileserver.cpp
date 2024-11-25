#include "tcpfileserver.h"

TcpFileServer::TcpFileServer(QWidget *parent)
    : QDialog(parent)
{
    totalBytes = 0;
    byteReceived = 0;
    fileNameSize = 0;

    // 初始化 UI
    serverProgressBar = new QProgressBar;
    serverStatusLabel = new QLabel(QStringLiteral("伺服器端就緒"));
    startButton = new QPushButton(QStringLiteral("接收"));
    quitButton = new QPushButton(QStringLiteral("退出"));

    buttonBox = new QDialogButtonBox;
    buttonBox->addButton(startButton, QDialogButtonBox::ActionRole);
    buttonBox->addButton(quitButton, QDialogButtonBox::RejectRole);

    // 添加輸入框
    ipInput = new QLineEdit;
    portInput = new QLineEdit;
    ipInput->setPlaceholderText(QStringLiteral("輸入伺服器 IP"));
    portInput->setPlaceholderText(QStringLiteral("輸入伺服器 Port"));

    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(new QLabel(QStringLiteral("IP:")));
    mainLayout->addWidget(ipInput);
    mainLayout->addWidget(new QLabel(QStringLiteral("Port:")));
    mainLayout->addWidget(portInput);
    mainLayout->addWidget(serverProgressBar);
    mainLayout->addWidget(serverStatusLabel);
    mainLayout->addStretch();
    mainLayout->addWidget(buttonBox);
    setLayout(mainLayout);

    setWindowTitle(QStringLiteral("接收檔案"));

    // 信號與槽的連接
    connect(startButton, SIGNAL(clicked()), this, SLOT(start()));
    connect(quitButton, SIGNAL(clicked()), this, SLOT(close()));
    connect(&tcpServer, SIGNAL(newConnection()), this, SLOT(acceptConnection()));
    connect(&tcpServer, SIGNAL(acceptError(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError)));
}

TcpFileServer::~TcpFileServer()
{
}

void TcpFileServer::start()
{
    startButton->setEnabled(false);
    byteReceived = 0;
    fileNameSize = 0;

    // 獲取用戶輸入的 IP 和 Port
    QString ipAddress = ipInput->text();
    quint16 port = portInput->text().toUInt();

    if (ipAddress.isEmpty() || port == 0)
    {
        QMessageBox::warning(this, QStringLiteral("錯誤"), QStringLiteral("請輸入有效的 IP 和 Port"));
        startButton->setEnabled(true);
        return;
    }

    QHostAddress hostAddress(ipAddress);

    // 啟動伺服器
    while (!tcpServer.isListening() && !tcpServer.listen(hostAddress, port))
    {
        QMessageBox::StandardButton ret = QMessageBox::critical(this,
                                                                QStringLiteral("錯誤"),
                                                                QStringLiteral("無法啟動伺服器: %1.").arg(tcpServer.errorString()),
                                                                QMessageBox::Retry | QMessageBox::Cancel);
        if (ret == QMessageBox::Cancel)
        {
            startButton->setEnabled(true);
            return;
        }
    }

    serverStatusLabel->setText(QStringLiteral("監聽中 (IP: %1, Port: %2)").arg(ipAddress).arg(port));
}

void TcpFileServer::acceptConnection()
{
    tcpServerConnection = tcpServer.nextPendingConnection(); // 取得客戶端連線
    connect(tcpServerConnection, SIGNAL(readyRead()), this, SLOT(updateServerProgress())); // 接收資料
    connect(tcpServerConnection, SIGNAL(error(QAbstractSocket::SocketError)),
            this, SLOT(displayError(QAbstractSocket::SocketError))); // 錯誤處理

    serverStatusLabel->setText(QStringLiteral("已接受連線"));
    tcpServer.close(); // 停止接受新的連線
}

void TcpFileServer::updateServerProgress()
{
    QDataStream in(tcpServerConnection);
    in.setVersion(QDataStream::Qt_4_6);

    if (byteReceived <= sizeof(qint64) * 2)
    {
        if ((fileNameSize == 0) && (tcpServerConnection->bytesAvailable() >= sizeof(qint64) * 2))
        {
            in >> totalBytes >> fileNameSize;
            byteReceived += sizeof(qint64) * 2;
        }
        if ((fileNameSize != 0) && (tcpServerConnection->bytesAvailable() >= fileNameSize))
        {
            in >> fileName;
            byteReceived += fileNameSize;
            localFile = new QFile(fileName);

            if (!localFile->open(QFile::WriteOnly))
            {
                QMessageBox::warning(this, QStringLiteral("錯誤"),
                                     QStringLiteral("無法開啟檔案 %1：%2").arg(fileName).arg(localFile->errorString()));
                return;
            }
        }
        else
        {
            return;
        }
    }

    if (byteReceived < totalBytes)
    {
        byteReceived += tcpServerConnection->bytesAvailable();
        inBlock = tcpServerConnection->readAll();
        localFile->write(inBlock);
        inBlock.resize(0);
    }

    serverProgressBar->setMaximum(totalBytes);
    serverProgressBar->setValue(byteReceived);

    serverStatusLabel->setText(QStringLiteral("已接收 %1 Bytes").arg(byteReceived));

    if (byteReceived == totalBytes)
    {
        tcpServerConnection->close();
        startButton->setEnabled(true);
        localFile->close();
    }
}

void TcpFileServer::displayError(QAbstractSocket::SocketError socketError)
{
    QMessageBox::information(this, QStringLiteral("網絡錯誤"),
                             QStringLiteral("產生錯誤: %1.").arg(tcpServer.errorString()));
    tcpServerConnection->close();
    serverProgressBar->reset();
    serverStatusLabel->setText(QStringLiteral("伺服器就緒"));
    startButton->setEnabled(true);
}

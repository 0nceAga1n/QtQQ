#pragma once
#include <QIODevice>
#include <QByteArray>
#include <QList>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QDataStream>

// 统一通信协议
// 帧格式：[4字节大端长度][JSON正文]
// JSON 消息由 buildChatMsg() 构造
namespace MsgProtocol {
    constexpr int TCP_PORT = 8888;

    // 字段名常量，避免手写字符串不一致
    const QString KEY_CMD = "cmd";
    const QString KEY_SENDER = "sender";
    const QString KEY_RECEIVER = "receiver";
    const QString KEY_GROUP = "group";
    const QString KEY_SEGMENTS = "segments";
    const QString KEY_TYPE = "type";
    const QString KEY_DATA = "data";

    // 把 JSON 对象打包成带长度前缀的字节流（发送端用）
    inline QByteArray pack(const QJsonObject& obj)
    {
        QByteArray body = QJsonDocument(obj).toJson(QJsonDocument::Compact);
        QByteArray frame;
        QDataStream stream(&frame, QIODevice::WriteOnly);
        stream.setByteOrder(QDataStream::BigEndian);
        stream << (quint32)body.size();   // 先写 4 字节长度
        frame.append(body);               // 再写正文
        return frame;
    }

    // 分帧解码器：累积字节流，循环吐出完整帧（接收端用）
    class Decoder
    {
    public:
        QList<QJsonObject> push(const QByteArray& data)
        {
            m_buffer.append(data);
            QList<QJsonObject> frames;
            while (m_buffer.size() >= 4) {
                QDataStream stream(m_buffer);
                stream.setByteOrder(QDataStream::BigEndian);
                quint32 len = 0;
                stream >> len;
                if (len == 0 || len > 100 * 1024 * 1024) {  // 允许单条消息最大 100MB，非法长度，丢弃保护
                    m_buffer.clear();
                    break;
                }
                if (m_buffer.size() < (int)(4 + len)) {      // 半包：数据不完整，等下一批
                    break;
                }
                QByteArray body = m_buffer.mid(4, len);
                m_buffer.remove(0, 4 + len);
                frames.append(QJsonDocument::fromJson(body).object());
            }
            return frames;
        }

    private:
        QByteArray m_buffer;
    };

    // 构造一条聊天消息 JSON
    inline QJsonObject buildChatMsg(const QString& sender,
        const QString& receiver,
        bool isGroup,
        const QJsonArray& segments)
    {
        QJsonObject obj;
        obj.insert("cmd", QString("msg"));
        obj.insert("sender", sender);
        obj.insert("receiver", receiver);
        obj.insert("group", isGroup ? 1 : 0);
        obj.insert("segments", segments);   // 每个元素 {type:text/image/file, data:...}
        return obj;
    }

    // 校验一条消息是否合法，非法直接丢弃
    inline bool isValidMsg(const QJsonObject& obj)
    {
        if (!obj.contains(KEY_SEGMENTS)) return false;
        QJsonArray segs = obj.value(KEY_SEGMENTS).toArray();
        if (segs.isEmpty() || segs.size() > 100) return false; // 空数组也拒绝
        for (const QJsonValue& v : segs) {
            QJsonObject seg = v.toObject();
            QString type = seg.value(KEY_TYPE).toString();
            QString data = seg.value(KEY_DATA).toString();
            if (type == "text") {
                if (data.size() > 4096) return false;   // 文本长度上限
            }
            else if (type == "image") {
                bool ok = false;
                int n = data.toInt(&ok);
                if (!ok || n < 0 || n > 170) return false;  // 表情编号范围
            }
            else if (type == "file") {
                quint32 size = seg.value("size").toInt();
                if (size > 100 * 1024 * 1024) return false; //文件大小上限100MB
            }
            else {
                return false;                           // 未知类型
            }
        }
        return true;
    }

} // namespace MsgProtocol

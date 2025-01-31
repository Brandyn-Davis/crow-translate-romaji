/*
 *  Copyright © 2018-2023 Hennadii Chernyshchyk <genaloner@gmail.com>
 *
 *  This file is part of Crow Translate.
 *
 *  Crow Translate is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  Crow Translate is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Crow Translate. If not, see <https://www.gnu.org/licenses/>.
 */

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include "translationedit.h"

#include "contextmenu.h"

class Romajify {
    public:
        Romajify()
	{
            setenv("PYTHONPATH", ".", 1);
            Py_Initialize();
        }

        ~Romajify()
	{
            Py_Finalize();
        }

        std::string romajify(const char* jpText)
	{
            pName = PyUnicode_FromString((char*)"romaji");
            pModule = PyImport_Import(pName);
            pDict = PyModule_GetDict(pModule);
            pFunc = PyDict_GetItemString(pDict, (char*)"romajify");
            pArgs = PyTuple_Pack(1, PyUnicode_FromString(jpText));
            
            if (PyCallable_Check(pFunc))
            	presult = PyObject_CallObject(pFunc, pArgs);
            else
            	return "[Ran into error!]";
            
            std::string result(PyUnicode_AsUTF8(presult));
            
            Py_XDECREF(presult);
            Py_XDECREF(pArgs);
            Py_XDECREF(pModule);
            Py_XDECREF(pName);
            
            return result;
        }

    private:
        PyObject *pName, *pModule, *pDict, *pFunc, *pArgs, *presult;
};

Romajify romajify;

TranslationEdit::TranslationEdit(QWidget *parent)
    : QTextEdit(parent)
{
}

bool TranslationEdit::parseTranslationData(QOnlineTranslator *translator)
{
    // Check for error
    if (translator->error() != QOnlineTranslator::NoError) {
        clearTranslation();
        setHtml(translator->errorString());
        emit translationDataParsed(translator->errorString());
        return false;
    }

    // Store translation information
    const bool translationWasEmpty = m_translation.isEmpty();
    m_translation = translator->translation();
    m_lang = translator->translationLanguage();

    // Remove bad chars
    // Note: this hack is here, because toHtml() can't render anything with utf-8 characters
    for (int i = 0; i < m_translation.size(); ++i) {
        if (m_translation[i].category() == QChar::Symbol_Other) {
            m_translation.remove(i, 1);
            --i;
        }
    }

    // Translation
    setHtml(m_translation.toHtmlEscaped().replace(QStringLiteral("\n"), QStringLiteral("<br>")));

    // Translit
    if (!translator->translationTranslit().isEmpty())
        append(QStringLiteral("<font color=\"grey\"><i>/%1/</i></font>").arg(translator->translationTranslit().replace(QStringLiteral("\n"), QStringLiteral("/<br>/"))));
    if (!translator->sourceTranslit().isEmpty())
        append(QStringLiteral("<font color=\"grey\"><i><b>(%1)</b></i></font>").arg(translator->sourceTranslit().replace(QStringLiteral("\n"), QStringLiteral("/<br>/"))));

    // Transcription
    if (!translator->sourceTranscription().isEmpty())
        append(QStringLiteral("<font color=\"grey\">[%1]</font>").arg(translator->sourceTranscription()));

    append({}); // Add new line before translation options

    // ROMAJI
    if (translator->sourceLanguage() == 48 && !translator->source().isEmpty()) // 48 represents japanese language
    {
        const char* result = romajify.romajify(translator->source().toLocal8Bit().data()).c_str();

        append(QStringLiteral("<font color=\"grey\">%1</font>").arg(tr("Romaji:")));

        QTextBlockFormat indent;
        indent.setTextIndent(20);
        textCursor().setBlockFormat(indent);
    
        append(QStringLiteral("<font color=\"grey\"><i>%1</i></font>").arg(result));

        indent.setTextIndent(0);
        textCursor().setBlockFormat(indent);
        append({});
    }


    // Translation options
    if (!translator->translationOptions().isEmpty()) {
        append(QStringLiteral("<font color=\"grey\"><i>%1</i> - %2</font>").arg(translator->source(), tr("translation options:")));

        // Print words for each type of speech
        const QMap<QString, QVector<QOption>> translationOptions = translator->translationOptions();
        for (auto it = translationOptions.cbegin(); it != translationOptions.cend(); ++it) {
            append(QStringLiteral("<b>%1</b>").arg(it.key()));
            QTextBlockFormat indent;
            indent.setTextIndent(20);
            textCursor().setBlockFormat(indent);

            for (const auto &[word, gender, translations] : it.value()) {
                // Show word gender
                QString wordLine;
                if (!gender.isEmpty())
                    wordLine.append(QStringLiteral("<i>%1</i> ").arg(gender));

                // Show Word
                wordLine.append(word);

                // Show word meaning
                if (!translations.isEmpty())
                    wordLine.append(QStringLiteral(": <font color=\"grey\"><i>%1</i></font>").arg(translations.join(QStringLiteral(", "))));

                // Add generated line to edit
                append(wordLine);
            }

            indent.setTextIndent(0);
            textCursor().setBlockFormat(indent);
            append({}); // Add a new line before the next type of speech
        }
    }

    // Examples
    if (!translator->examples().isEmpty()) {
        append(QStringLiteral("<font color=\"grey\"><i>%1</i> - %2</font>").arg(translator->source(), tr("examples:")));
        const QMap<QString, QVector<QExample>> examples = translator->examples();
        for (auto it = examples.cbegin(); it != examples.cend(); ++it) {
            append(QStringLiteral("<b>%1</b>").arg(it.key()));
            QTextBlockFormat indent;
            indent.setTextIndent(20);
            textCursor().setBlockFormat(indent);
            for (const auto &[example, description] : it.value()) {
                append(description);
                append(QStringLiteral("<font color=\"grey\"><i>%1</i></font>").arg(example));
                append({});
            }
            indent.setTextIndent(0);
            textCursor().setBlockFormat(indent);
        }
    }

    moveCursor(QTextCursor::Start);
    emit translationDataParsed(toHtml());
    if (translationWasEmpty)
        emit translationEmpty(false);
    return true;
}

QString TranslationEdit::translation() const
{
    return m_translation;
}

QOnlineTranslator::Language TranslationEdit::translationLanguage()
{
    return m_lang;
}

void TranslationEdit::clearTranslation()
{
    if (!m_translation.isEmpty()) {
        m_translation.clear();
        m_lang = QOnlineTranslator::NoLanguage;
        emit translationEmpty(true);
    }
    clear();
}

void TranslationEdit::contextMenuEvent(QContextMenuEvent *event)
{
    auto *contextMenu = new ContextMenu(this, event);
    contextMenu->popup();
}

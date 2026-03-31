#include "xterm.hpp"

#include "../wxTerminal.hpp"

namespace os::term {

XtermInterpreter::XtermInterpreter(wxTerminal& term) : m_term(term) {}

void XtermInterpreter::Reset() {
    m_esc = EscState::Ground;
    m_csiParams.clear();
    m_csiParamCur = 0;
}

int XtermInterpreter::ParamAt(const std::vector<int>& params, size_t i, int defVal) {
    if (params.empty())
        return defVal;
    if (i >= params.size())
        return defVal;
    int v = params[i];
    if (v < 0)
        return defVal;
    if (v == 0)
        return defVal;
    return v;
}

bool XtermInterpreter::EvalCsi(char finalCh, bool question) {
    if (question) {
        if (finalCh == 'h' || finalCh == 'l') {
            if (m_csiParams.size() == 1 && m_csiParams[0] == 25)
                m_term.SetCursorVisible(finalCh == 'h');
        }
        return true;
    }

    switch (finalCh) {
    case 'm': {
        std::vector<int> args = m_csiParams;
        if (args.empty())
            args.push_back(0);
        m_term.ApplySgr(args);
        break;
    }
    case 'H':
    case 'f':
        m_term.SetCursorPosition(ParamAt(m_csiParams, 0, 1), ParamAt(m_csiParams, 1, 1));
        break;
    case 'A':
        m_term.GoUp(ParamAt(m_csiParams, 0, 1));
        break;
    case 'B':
        m_term.GoDown(ParamAt(m_csiParams, 0, 1));
        break;
    case 'C':
        m_term.GoRight(ParamAt(m_csiParams, 0, 1));
        break;
    case 'D':
        m_term.GoLeft(ParamAt(m_csiParams, 0, 1));
        break;
    case 'G':
        m_term.SetCursorColumn(ParamAt(m_csiParams, 0, 1));
        break;
    case 'd':
        m_term.SetCursorRow(ParamAt(m_csiParams, 0, 1));
        break;
    case 'J':
        m_term.EraseInDisplay(ParamAt(m_csiParams, 0, 0));
        break;
    case 'K':
        m_term.EraseInLine(ParamAt(m_csiParams, 0, 0));
        break;
    default:
        break;
    }
    return true;
}

bool XtermInterpreter::Receive(uint32_t cp) {
    const auto isCsiFinal = [](uint32_t c) { return c >= 0x40u && c <= 0x7eu; };

    if (m_esc == EscState::Ground) {
        if (cp == 27u) {
            m_esc = EscState::Esc;
            m_csiParams.clear();
            m_csiParamCur = 0;
            return true;
        }
        return false;
    }

    if (m_esc == EscState::Esc) {
        if (cp == static_cast<uint32_t>('[')) {
            m_esc = EscState::Csi;
            m_csiParams.clear();
            m_csiParams.push_back(0);
            m_csiParamCur = 0;
            return true;
        }
        m_term.EscSecond(static_cast<uint8_t>(cp & 0xffu));
        m_esc = EscState::Ground;
        return true;
    }

    if (m_esc == EscState::Csi) {
        if (cp == static_cast<uint32_t>('?')) {
            m_esc = EscState::CsiQuest;
            m_csiParams.clear();
            m_csiParams.push_back(0);
            m_csiParamCur = 0;
            return true;
        }
        if (cp >= static_cast<uint32_t>('0') && cp <= static_cast<uint32_t>('9')) {
            m_csiParams[static_cast<size_t>(m_csiParamCur)] =
                m_csiParams[static_cast<size_t>(m_csiParamCur)] * 10 +
                static_cast<int>(cp - static_cast<uint32_t>('0'));
            return true;
        }
        if (cp == static_cast<uint32_t>(';')) {
            m_csiParams.push_back(0);
            m_csiParamCur = static_cast<int>(m_csiParams.size()) - 1;
            return true;
        }
        if (isCsiFinal(cp)) {
            EvalCsi(static_cast<char>(cp), false);
            m_esc = EscState::Ground;
            return true;
        }
        m_esc = EscState::Ground;
        return false;
    }

    if (m_esc == EscState::CsiQuest) {
        if (cp >= static_cast<uint32_t>('0') && cp <= static_cast<uint32_t>('9')) {
            m_csiParams[static_cast<size_t>(m_csiParamCur)] =
                m_csiParams[static_cast<size_t>(m_csiParamCur)] * 10 +
                static_cast<int>(cp - static_cast<uint32_t>('0'));
            return true;
        }
        if (cp == static_cast<uint32_t>(';')) {
            m_csiParams.push_back(0);
            m_csiParamCur = static_cast<int>(m_csiParams.size()) - 1;
            return true;
        }
        if (isCsiFinal(cp)) {
            EvalCsi(static_cast<char>(cp), true);
            m_esc = EscState::Ground;
            return true;
        }
        m_esc = EscState::Ground;
        return false;
    }

    return false;
}

} // namespace os::term

#pragma once

#include "resource.h"

#include <optional>
#include <string_view>

namespace AutoItPlus::Editor
{
    inline std::optional<int> FindEmbeddedIconResourceId(std::string_view iconName)
    {
        if (iconName == "123") return IDR_ICON_123;
        if (iconName == "arrow-left-circle-fill") return IDR_ICON_ARROW_LEFT_CIRCLE_FILL;
        if (iconName == "arrow-right-circle-fill") return IDR_ICON_ARROW_RIGHT_CIRCLE_FILL;
        if (iconName == "braces-asterisk") return IDR_ICON_BRACES_ASTERISK;
        if (iconName == "braces") return IDR_ICON_BRACES;
        if (iconName == "code-square") return IDR_ICON_CODE_SQUARE;
        if (iconName == "eye-fill") return IDR_ICON_EYE_FILL;
        if (iconName == "file-code") return IDR_ICON_FILE_CODE;
        if (iconName == "file-earmark-fill") return IDR_ICON_FILE_EARMARK_FILL;
        if (iconName == "file-earmark-plus-fill") return IDR_ICON_FILE_EARMARK_PLUS_FILL;
        if (iconName == "files") return IDR_ICON_FILES;
        if (iconName == "filetype-bmp") return IDR_ICON_FILETYPE_BMP;
        if (iconName == "filetype-css") return IDR_ICON_FILETYPE_CSS;
        if (iconName == "filetype-csv") return IDR_ICON_FILETYPE_CSV;
        if (iconName == "filetype-exe") return IDR_ICON_FILETYPE_EXE;
        if (iconName == "filetype-gif") return IDR_ICON_FILETYPE_GIF;
        if (iconName == "filetype-heic") return IDR_ICON_FILETYPE_HEIC;
        if (iconName == "filetype-html") return IDR_ICON_FILETYPE_HTML;
        if (iconName == "filetype-java") return IDR_ICON_FILETYPE_JAVA;
        if (iconName == "filetype-jpg") return IDR_ICON_FILETYPE_JPG;
        if (iconName == "filetype-js") return IDR_ICON_FILETYPE_JS;
        if (iconName == "filetype-json") return IDR_ICON_FILETYPE_JSON;
        if (iconName == "filetype-jsx") return IDR_ICON_FILETYPE_JSX;
        if (iconName == "filetype-md") return IDR_ICON_FILETYPE_MD;
        if (iconName == "filetype-mdx") return IDR_ICON_FILETYPE_MDX;
        if (iconName == "filetype-mp3") return IDR_ICON_FILETYPE_MP3;
        if (iconName == "filetype-mp4") return IDR_ICON_FILETYPE_MP4;
        if (iconName == "filetype-pdf") return IDR_ICON_FILETYPE_PDF;
        if (iconName == "filetype-php") return IDR_ICON_FILETYPE_PHP;
        if (iconName == "filetype-png") return IDR_ICON_FILETYPE_PNG;
        if (iconName == "filetype-psd") return IDR_ICON_FILETYPE_PSD;
        if (iconName == "filetype-py") return IDR_ICON_FILETYPE_PY;
        if (iconName == "filetype-rb") return IDR_ICON_FILETYPE_RB;
        if (iconName == "filetype-sass") return IDR_ICON_FILETYPE_SASS;
        if (iconName == "filetype-scss") return IDR_ICON_FILETYPE_SCSS;
        if (iconName == "filetype-sh") return IDR_ICON_FILETYPE_SH;
        if (iconName == "filetype-sql") return IDR_ICON_FILETYPE_SQL;
        if (iconName == "filetype-svg") return IDR_ICON_FILETYPE_SVG;
        if (iconName == "filetype-tiff") return IDR_ICON_FILETYPE_TIFF;
        if (iconName == "filetype-tsx") return IDR_ICON_FILETYPE_TSX;
        if (iconName == "filetype-ttf") return IDR_ICON_FILETYPE_TTF;
        if (iconName == "filetype-txt") return IDR_ICON_FILETYPE_TXT;
        if (iconName == "filetype-wav") return IDR_ICON_FILETYPE_WAV;
        if (iconName == "filetype-woff") return IDR_ICON_FILETYPE_WOFF;
        if (iconName == "filetype-xls") return IDR_ICON_FILETYPE_XLS;
        if (iconName == "filetype-xlsx") return IDR_ICON_FILETYPE_XLSX;
        if (iconName == "filetype-xml") return IDR_ICON_FILETYPE_XML;
        if (iconName == "filetype-yml") return IDR_ICON_FILETYPE_YML;
        if (iconName == "folder-fill") return IDR_ICON_FOLDER_FILL;
        if (iconName == "folder-plus") return IDR_ICON_FOLDER_PLUS;
        if (iconName == "folder2-open") return IDR_ICON_FOLDER2_OPEN;
        if (iconName == "gear-fill") return IDR_ICON_GEAR_FILL;
        if (iconName == "github") return IDR_ICON_GITHUB;
        if (iconName == "globe") return IDR_ICON_GLOBE;
        if (iconName == "hash") return IDR_ICON_HASH;
        if (iconName == "journal-code") return IDR_ICON_JOURNAL_CODE;
        if (iconName == "journal") return IDR_ICON_JOURNAL;
        if (iconName == "key-fill") return IDR_ICON_KEY_FILL;
        if (iconName == "list-nested") return IDR_ICON_LIST_NESTED;
        if (iconName == "list") return IDR_ICON_LIST;
        if (iconName == "magic") return IDR_ICON_MAGIC;
        if (iconName == "play-fill") return IDR_ICON_PLAY_FILL;
        if (iconName == "rulers") return IDR_ICON_RULERS;
        if (iconName == "save") return IDR_ICON_SAVE;
        if (iconName == "star-fill") return IDR_ICON_STAR_FILL;
        if (iconName == "terminal-fill") return IDR_ICON_TERMINAL_FILL;
        if (iconName == "type") return IDR_ICON_TYPE;
        if (iconName == "window-sidebar") return IDR_ICON_WINDOW_SIDEBAR;
        if (iconName == "zoom-in") return IDR_ICON_ZOOM_IN;
        if (iconName == "zoom-out") return IDR_ICON_ZOOM_OUT;
        return std::nullopt;
    }
}

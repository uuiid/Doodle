#pragma once
#include <string>
namespace doodle::logger {
const static std::string logTemplate =
    R"logTemplate(<?xml version="1.0" encoding="utf-8" standalone="yes"?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Strict//EN" "https://github.com/">
<html>

<head>
    <title>TaoLogger</title>
    <meta http-equiv="Content-Type" content="text/html; charset=utf-8" />
    <style type="text/css" id="logCss">
        body {
            background: #18242b;
            color: #afc6d1;
            margin-right: 20px;
            margin-left: 20px;
            font-size: 14px;
            font-family: Arial, sans-serif, sans;
            /* white-space: pre-wrap; */
        }

        a {
            text-decoration: none;
        }

        a:link {
            color: #a0b2bb;
        }

        a:active {
            color: #f59504;
        }

        a:visited {
            color: #adc7d4;
        }

        a:hover {
            color: #e49115;
        }

        h1 {
            text-align: center;
        }

        h2 {
            color: #ebe5e5;
        }

        .debug,
        .warning,
        .trace,
        .fatal,
        .info {
            padding: 3px;
            overflow: auto;
        }

        .debug {
            background-color: #0f1011;
            color: #a8c1ce;
        }

        .info {
            background-color: #294453;
            color: #a8c1ce;
        }

        .warning {
            background-color: #7993a0;
            color: #1b2329;
        }

        .trace {
            background-color: #ff952b;
            color: #1d2930;
        }

        .fatal {
            background-color: #fc0808;
            color: #19242b;
        }
    </style>
</head>

<body>
    <h1><a href="https://github.com/">TaoLogger</a> 日志文件</h1>
    <script type="text/JavaScript">
        function objHide(obj) {
            obj.style.display="none"
        }
        function objShow(obj) {
            obj.style.display="block";
            // obj.style.whiteSpace = "pre-wrap";
        }
        function selectType() {
            var sel = document.getElementById("typeSelect");
            const hideList = new Set(['all','trace', 'debug', 'info', 'warning', 'error','fatal']);
            if (sel.value === 'all') {
                hideList.forEach(element => {
                    var list = document.querySelectorAll('.' + element);
                    list.forEach(objShow);
                });
            } else {
                var ss = hideList;
                ss.delete(sel.value);
                ss.forEach(element => {
                    var list = document.querySelectorAll('.' + element);
                    list.forEach(objHide);
                });
                var showList = document.querySelectorAll('.' + sel.value);
                showList.forEach(objShow);
            }
        }
    </script>
    <select id="typeSelect" onchange="selectType()">
        <option value='all' selected="selected">All</option>
        <option value='debug'>Debug</option>
        <option value='info'>Info</option>
        <option value='warning'>Warning</option>
        <option value='error'>error</option>
        <option value='trace'>Critical</option>
        <option value='fatal'>Fatal</option>
    </select>
)logTemplate";
}

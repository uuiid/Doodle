main_style = """

    QWidget {
        color: rgb(221, 221, 221);
        font: 10pt "Segoe" UI;
    
    }
    
    QSplitter::handle {
        background: rgba(0,0,0,0);
    }
    QSplitter::handle:horizontal {
        background-color: rgba(0,0,0,0);
    }
    QSplitter::handle:vertical {
        background-color: rgba(0,0,0,0);
    }
    QScrollArea {
        border:0px solid rgb(255,255,255,0.4);
        background-color: transparent;
    }
    
    QScrollArea:bar {
        background-color: rgba(0,0,0,0);
    }
    
    QPushButton {
        border-radius: 5px;
        background-color: #5d5d5d;
        padding: 5px;
        cursor: pointer;
    }
    
    QPushButton:hover {
        background-color: #596d7a;
    }
    
    #MainWidget {
        background-color: #444444;
    }
    #content_widget {
        border:1px solid rgb(0,0,0,1);
        border-radius:10px;
    }
    #scroll-area {
        border:1px solid rgb(0,0,0,0);
        background-color: transparent;
    }
    #sidebar-item {
        background-color: rgba(255, 255, 255,0);
    }
    #sidebar-item:hover {
        background-color: #5285a6;
        color: #298DFF;
    }
    
    #sidebar-item-light:hover {
        background-color: rgba(255,255,255,0.1);
        color: black;
    }
    
    QLabel {
        padding-left: 10px;
        padding-right: 10px;
    }
    #doodle-scroll-area {
        border:1px solid rgb(0,0,0,1);
        border-radius:10px;
        background-color: #373737;
    }
    
    #doodle-scroll-area-light {
        background-color: rgba(255,255,255,0.1);
    }
    #doodle-scroll-area-t {
        background-color: transparent;
    }
    
    #create-cloth-model-item {
        background-color: #373737;
        border-radius:10px;
    }
    
    #create-cloth-model-item:hover {
        border-radius:10px;
        background-color: rgba(255,255,255,0.1);
        color: black;
    }
    
    
    #high-model-widget-item {
        border-radius:10px;
        background-color: #363636;
    
    }
    
    #high-model-widget-item:hover {
        border-radius:10px;
        background-color: #5285a6;
    
    }

"""

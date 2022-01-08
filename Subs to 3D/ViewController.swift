//
//  ViewController.swift
//  Subs to 3D
//
//  Created by Konstantin Tuev on 8.01.22.
//

import Cocoa

class ViewController: NSViewController {
    
    @IBOutlet weak var SideBySide: NSButton!
    @IBOutlet weak var TopBottom: NSButton!
    @IBOutlet weak var ErrorLabel: NSTextField!
    
    enum SubConvertType: Int {
        case ZNSUB_ASS3D_SBS = 1, ZNSUB_ASS3D_TB, ZNSUB_ASS3D_NO3D
    }
    
    var subsConvert: SubConvertType = SubConvertType.ZNSUB_ASS3D_SBS;

    override func viewDidLoad() {
        super.viewDidLoad()
        // Do any additional setup after loading the view.
    }
    @IBAction func clickedRadioButton(_ sender: NSButton) {
        if (SideBySide.state == NSControl.StateValue.on) {
            subsConvert = SubConvertType.ZNSUB_ASS3D_SBS;
        } else {
            subsConvert = SubConvertType.ZNSUB_ASS3D_TB;
        }
    }
    
    override var representedObject: Any? {
        didSet {
        // Update the view, if already loaded.
        }
    }
    @IBAction func clickedConvert(_ sender: Any) {
        openFilePicker();
    }
    
    func openFilePicker() {
        ErrorLabel.stringValue = "";
        let dialog = NSOpenPanel();

        dialog.title                   = "Choose an input subtitle file (NOT 3D)";
        dialog.message                 = "Choose an input subtitle file (NOT 3D)";
        dialog.showsResizeIndicator    = true;
        dialog.showsHiddenFiles        = false;
        dialog.allowsMultipleSelection = false;
        dialog.canChooseDirectories = false;
        dialog.allowedFileTypes        = ["srt", "ass"];

        if (dialog.runModal() ==  NSApplication.ModalResponse.OK) {
            let result = dialog.url;

            if (result != nil) {
                let path: String = result!.path
                convertSubsTo3D(input: path)
            }
            
        }
    }
    
    func convertSubsTo3D(input: String) {
        let url = URL(fileURLWithPath: input)
        
        let dialog = NSSavePanel()

        dialog.level = .modalPanel
        dialog.title                    = "Choose a destination for 3D subtitles"
        dialog.nameFieldStringValue     = url.deletingPathExtension().lastPathComponent + ".ass"
        dialog.showsResizeIndicator     = true
        dialog.showsHiddenFiles         = false
        dialog.canCreateDirectories     = true
        dialog.message                  = "Hint: Click on the movie to match it's name and location"
        dialog.allowedFileTypes         = ["ass"]

        if (dialog.runModal() == .OK) {
            // Pathname of the file
            if let output = dialog.url?.path {
                let result = converSubsTo3dSubs(input, output, Int32(subsConvert.rawValue));
                if (result == nil) {
                    ErrorLabel.stringValue = "Unknown Error occurred!";
                    ErrorLabel.textColor = NSColor.systemRed;
                    return;
                }
                
                let error = String(cString: result!)
                if (!error.isEmpty) {
                    ErrorLabel.stringValue = error;
                    ErrorLabel.textColor = NSColor.systemRed;
                    return;
                }
                ErrorLabel.stringValue = "Done";
                ErrorLabel.textColor = NSColor.systemGreen;
            }
        }
    }
}


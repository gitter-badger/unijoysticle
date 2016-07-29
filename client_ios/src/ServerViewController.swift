/****************************************************************************
 http://retro.moe/unijoysticle

 Copyright 2016 Ricardo Quesada

 Licensed under the Apache License, Version 2.0 (the "License");
 you may not use this file except in compliance with the License.
 You may obtain a copy of the License at

 http://www.apache.org/licenses/LICENSE-2.0

 Unless required by applicable law or agreed to in writing, software
 distributed under the License is distributed on an "AS IS" BASIS,
 WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 See the License for the specific language governing permissions and
 limitations under the License.
 ****************************************************************************/

import UIKit

class ServerViewController: UIViewController, UITextFieldDelegate {

    @IBOutlet weak var ipaddress: UITextField!

    @IBOutlet weak var handicapSlider: UISlider!
    @IBOutlet weak var handicapLabel: UILabel!

    @IBOutlet weak var jumpSlider: UISlider!
    @IBOutlet weak var jumpLabel: UILabel!

    @IBOutlet weak var movementSlider: UISlider!
    @IBOutlet weak var movementLabel: UILabel!

    let sliderStep:Float = 0.1

    override func viewDidLoad() {
        super.viewDidLoad()
        let settings = NSUserDefaults.standardUserDefaults()

        // ip address
        let addr = settings.valueForKey("ipaddress")
        if (addr != nil) {
            ipaddress.text = addr as! String?
        }
        else {
            ipaddress.text = "192.168.4.1"
        }

        // handicap
        let handicapValue = settings.valueForKey("handicap")
        var handiFloat:Float = 1.0
        if (handicapValue != nil) {
            handiFloat = handicapValue as! Float
        }
        handicapSlider.setValue(handiFloat, animated: false)
        handicapLabel.text = "\(handiFloat)"

        // movement threshold
        let movementValue = settings.valueForKey("movement threshold")
        var movementFloat:Float = 0.4
        if (movementValue != nil) {
            movementFloat = movementValue as! Float
        }
        movementSlider.setValue(movementFloat, animated: false)
        movementLabel.text = "\(movementFloat)"

        // jump threshold
        let jumpValue = settings.valueForKey("jump threshold")
        var jumpFloat:Float = 2.1
        if (jumpValue != nil) {
            jumpFloat = jumpValue as! Float
        }
        jumpSlider.setValue(jumpFloat, animated: false)
        jumpLabel.text = "\(jumpFloat)"
    }

    func textFieldShouldReturn(textField: UITextField) -> Bool {
        let settings = NSUserDefaults.standardUserDefaults()
        settings.setValue(textField.text, forKey: "ipaddress")
        textField.resignFirstResponder()
        return true
    }

    @IBAction func sliderValueChanged(sender: UISlider) {
        let steppedValue = round(handicapSlider.value / sliderStep) * sliderStep
        handicapLabel.text = "\(steppedValue)"
        handicapSlider.value = steppedValue
        let settings = NSUserDefaults.standardUserDefaults()
        settings.setValue(steppedValue, forKey: "handicap")
    }

    @IBAction func movementValueChanged(sender: AnyObject) {
        let steppedValue = round(movementSlider.value / sliderStep) * sliderStep
        movementLabel.text = "\(steppedValue)"
        movementSlider.value = steppedValue
        let settings = NSUserDefaults.standardUserDefaults()
        settings.setValue(steppedValue, forKey: "movement threshold")
    }
    @IBAction func jumpValueChanged(sender: AnyObject) {
        let steppedValue = round(jumpSlider.value / sliderStep) * sliderStep
        jumpLabel.text = "\(steppedValue)"
        jumpSlider.value = steppedValue
        let settings = NSUserDefaults.standardUserDefaults()
        settings.setValue(steppedValue, forKey: "jump threshold")
    }
}
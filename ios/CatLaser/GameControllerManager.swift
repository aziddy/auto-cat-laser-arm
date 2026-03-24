import Foundation
import GameController
import WebKit

@MainActor
class GameControllerManager: ObservableObject {
    @Published var controllerName: String?
    @Published var controllerEnabled = true
    @Published var relativeMode = false

    weak var webView: WKWebView?
    var isPageReady = false

    private var pollTimer: Timer?
    private let deadzone: Float = 0.15

    init() {
        NotificationCenter.default.addObserver(
            self, selector: #selector(controllerDidConnect),
            name: .GCControllerDidConnect, object: nil
        )
        NotificationCenter.default.addObserver(
            self, selector: #selector(controllerDidDisconnect),
            name: .GCControllerDidDisconnect, object: nil
        )

        // Check if a controller is already connected at launch
        if let controller = GCController.controllers().first {
            print("[CatLaser] Controller already connected at init: \(controller.vendorName ?? "unknown")")
            controllerConnected(controller)
        }
    }

    @objc private func controllerDidConnect(_ note: Notification) {
        guard let controller = note.object as? GCController else { return }
        print("[CatLaser] Controller connected via notification: \(controller.vendorName ?? "unknown")")
        controllerConnected(controller)
    }

    private func controllerConnected(_ controller: GCController) {
        controllerName = controller.vendorName ?? "Game Controller"

        // Set up valueChangedHandler — fires on any button/stick change
        if let gamepad = controller.extendedGamepad {
            print("[CatLaser] extendedGamepad available, setting up valueChangedHandler")
            gamepad.valueChangedHandler = { [weak self] gamepad, element in
                Task { @MainActor in
                    self?.handleGamepadInput(gamepad)
                }
            }
        } else {
            print("[CatLaser] WARNING: no extendedGamepad on this controller")
        }

        startPolling()
    }

    @objc private func controllerDidDisconnect(_ note: Notification) {
        print("[CatLaser] Controller disconnected")
        controllerName = nil
        stopPolling()
    }

    private func startPolling() {
        stopPolling()
        print("[CatLaser] Starting poll timer (isPageReady=\(isPageReady), webView=\(webView != nil ? "set" : "nil"))")
        let timer = Timer(timeInterval: 1.0 / 30.0, repeats: true) { [weak self] _ in
            Task { @MainActor in
                self?.pollController()
            }
        }
        RunLoop.main.add(timer, forMode: .common)
        pollTimer = timer
    }

    private func stopPolling() {
        pollTimer?.invalidate()
        pollTimer = nil
    }

    private func handleGamepadInput(_ gamepad: GCExtendedGamepad) {
        injectStickValues(gamepad)
    }

    private func pollController() {
        guard let controller = GCController.controllers().first,
              let gamepad = controller.extendedGamepad else { return }
        injectStickValues(gamepad)
    }

    private func injectStickValues(_ gamepad: GCExtendedGamepad) {
        guard controllerEnabled else { return }
        guard isPageReady else {
            print("[CatLaser] Skipping: page not ready")
            return
        }
        guard let webView else {
            print("[CatLaser] Skipping: webView is nil")
            return
        }

        var lx = gamepad.leftThumbstick.xAxis.value
        var ly = gamepad.leftThumbstick.yAxis.value

        // Apply deadzone
        if abs(lx) < deadzone { lx = 0 }
        if abs(ly) < deadzone { ly = 0 }

        // In relative mode, skip when stick is centered (no movement needed)
        if relativeMode && lx == 0 && ly == 0 { return }

        let js: String
        if relativeMode {
            js = "if(window.setAnglesFromStickRelative)window.setAnglesFromStickRelative(\(lx),\(ly),2)"
        } else {
            js = "if(window.setAnglesFromStick)window.setAnglesFromStick(\(lx),\(ly))"
        }
        webView.evaluateJavaScript(js) { _, error in
            if let error { print("[CatLaser] JS error: \(error)") }
        }
    }

    deinit {
        pollTimer?.invalidate()
    }
}

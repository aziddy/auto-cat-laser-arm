import SwiftUI
import NetworkExtension

struct ContentView: View {
    @StateObject private var gameController = GameControllerManager()
    @State private var pageLoaded = false

    var body: some View {
        ZStack {
            WebViewRepresentable(
                url: URL(string: "http://192.168.4.1/")!,
                onWebViewCreated: { webView in
                    gameController.webView = webView
                },
                onPageReady: {
                    pageLoaded = true
                    gameController.isPageReady = true
                },
                onLoadFailed: {
                    pageLoaded = false
                    gameController.isPageReady = false
                }
            )
            .ignoresSafeArea()

            if !pageLoaded {
                VStack(spacing: 16) {
                    ProgressView()
                        .tint(.gray)
                    Text("Connect to CatLaser WiFi")
                        .font(.headline)
                        .foregroundStyle(.white)
                    Text("Settings → Wi-Fi → CatLaser\nPassword: pew-pew-pew")
                        .font(.subheadline)
                        .foregroundStyle(.gray)
                        .multilineTextAlignment(.center)
                    Button("Join CatLaser WiFi") {
                        let config = NEHotspotConfiguration(ssid: "CatLaser", passphrase: "pew-pew-pew", isWEP: false)
                        config.joinOnce = false
                        NEHotspotConfigurationManager.shared.apply(config) { error in
                            if error == nil || (error as? NSError)?.code == NEHotspotConfigurationError.alreadyAssociated.rawValue {
                                // Connected or already on CatLaser — reload
                                DispatchQueue.main.asyncAfter(deadline: .now() + 1) {
                                    gameController.webView?.load(URLRequest(url: URL(string: "http://192.168.4.1/")!))
                                }
                            }
                        }
                    }
                    .buttonStyle(.borderedProminent)
                    .tint(.blue)
                    Button("Try Again") {
                        if let webView = gameController.webView {
                            webView.load(URLRequest(url: URL(string: "http://192.168.4.1/")!))
                        }
                    }
                    .buttonStyle(.bordered)
                    .tint(.gray)
                }
                .frame(maxWidth: .infinity, maxHeight: .infinity)
                .background(Color(red: 0.1, green: 0.1, blue: 0.18))
            }

            if pageLoaded {
                VStack {
                    HStack {
                        Button(action: {
                            gameController.webView?.reload()
                        }) {
                            Image(systemName: "arrow.clockwise")
                                .font(.caption)
                                .padding(8)
                                .background(.ultraThinMaterial)
                                .cornerRadius(8)
                        }
                        .padding(.top, 50)
                        .padding(.leading, 12)

                        Spacer()

                        if gameController.controllerName != nil && gameController.controllerEnabled {
                            Button(action: {
                                gameController.relativeMode.toggle()
                            }) {
                                Text(gameController.relativeMode ? "REL" : "ABS")
                                    .font(.caption2.bold())
                                    .padding(.horizontal, 8)
                                    .padding(.vertical, 4)
                                    .background(.ultraThinMaterial)
                                    .cornerRadius(8)
                            }
                            .padding(.top, 50)
                        }

                        if let name = gameController.controllerName {
                            Button(action: {
                                gameController.controllerEnabled.toggle()
                            }) {
                                HStack(spacing: 6) {
                                    Image(systemName: gameController.controllerEnabled ? "gamecontroller.fill" : "gamecontroller")
                                        .font(.caption2)
                                    Text(name)
                                        .font(.caption2)
                                }
                                .padding(.horizontal, 8)
                                .padding(.vertical, 4)
                                .background(.ultraThinMaterial)
                                .cornerRadius(8)
                                .opacity(gameController.controllerEnabled ? 1.0 : 0.5)
                            }
                            .padding(.top, 50)
                            .padding(.trailing, 12)
                        }
                    }
                    Spacer()
                }
            }
        }
        .preferredColorScheme(.dark)
    }
}

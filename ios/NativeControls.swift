import SwiftUI
import UIKit

private struct HardwareButton: View {
  let symbol: String
  let label: String
  let pressed: (Bool) -> Void

  var body: some View {
    Button {
      pressed(true)
      pressed(false)
    } label: {
      Image(systemName: symbol)
    }
    .buttonStyle(.bordered)
    .accessibilityLabel(label)
  }
}

private struct SimulatorControls: View {
  var body: some View {
    HStack {
      Button(action: simPlatformPickFolder) {
        Image(systemName: "folder")
      }
      .buttonStyle(.bordered)
      .accessibilityLabel("Choose library folder")

      if simControlHasTouch() {
        button("chevron.up", "Previous page", 4)
        if simControlHasHomeKey() {
          HardwareButton(symbol: "circle", label: "Home", pressed: simControlSetHomeKey)
        }
        button("power", "Power", 6)
        button("chevron.down", "Next page", 5)
      } else {
        button("arrow.uturn.backward", "Back", 0)
        button("chevron.left", "Left", 2)
        button("chevron.up", "Up", 4)
        button("checkmark", "Confirm", 1)
        button("chevron.down", "Down", 5)
        button("chevron.right", "Right", 3)
        button("power", "Power", 6)
      }
    }
  }

  private func button(_ symbol: String, _ label: String, _ index: Int32) -> some View {
    HardwareButton(symbol: symbol, label: label) { down in
      simControlSetButton(index, down)
    }
  }
}

private var controlsController: UIViewController?

@_cdecl("simPlatformInstallControls")
func simPlatformInstallControls() {
  DispatchQueue.main.async {
    guard controlsController == nil,
      let window = UIApplication.shared.connectedScenes
        .compactMap({ $0 as? UIWindowScene })
        .flatMap(\.windows)
        .first(where: \.isKeyWindow),
      let root = window.rootViewController
    else { return }

    let hosting = UIHostingController(rootView: SimulatorControls())
    hosting.sizingOptions = .intrinsicContentSize
    hosting.view.backgroundColor = .clear
    hosting.view.translatesAutoresizingMaskIntoConstraints = false

    root.addChild(hosting)
    root.view.addSubview(hosting.view)
    NSLayoutConstraint.activate([
      hosting.view.centerXAnchor.constraint(equalTo: root.view.centerXAnchor),
      hosting.view.bottomAnchor.constraint(
        equalTo: root.view.safeAreaLayoutGuide.bottomAnchor, constant: -8),
    ])
    hosting.didMove(toParent: root)
    controlsController = hosting
  }
}

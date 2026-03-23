# How to Deploy

## Directly onto the Watch

### Copy Your App into the Watch Memory via USB

1. Connect the watch via USB cable. Wait for Mass Storage to be attached. Note: It may take some time as running apps may need to flush their data before mass storage becomes active.
2. Go to the `Apps/` directory and create a new one with a name equal to the app name.
3. Paste the `*.uapp` into this folder.
4. Eject the attached watch drive **safely**.
5. Disconnect the USB cable.
6. Perform a watch power cycle: power off the watch and turn it on again.
7. Press the top right button and check the app.

### Troubleshooting

- If you do not see the app, check the hash sum of the copied file.
- If the error still persists, use a **debug** adapter to monitor the watch debug UART Tx line to see the logs.
- If you still encounter any platform issues, please create an issue at the [GitHub project issue page](https://github.com/UNAWatch/una-sdk/issues).
- To discuss any issues, use [Discussions](https://github.com/UNAWatch/una-sdk/discussions).

## Sharing the Apps

### Via https://apps.unawatch.com for Closed Source Apps

**Disclaimer:** In these instructional steps, we use the __Files__ Tutorial **as an example**. Please apply the steps below to your own app!

- Enter the [portal page](https://apps.unawatch.com) and sign up.

  ![Login](assets/login.png)

- After signing in, click **Add New**.

  ![Main](assets/main-page.png)

- Enter **App Name** and a brief description. Click the Generate button.

  ![Add New App](assets/add-app.png)

- Copy **App ID** from app Page:

  ![New App Page](assets/app-page.png)

- Paste it into your [`CMakeLists.txt`](CMakeLists.txt) into the `APP_ID` variable. **Note:** APP_ID is required to track the apps in the apps store and for the mobile app to match new `*.uapp` file versions in case the file name itself has been changed.

  ![App ID](assets/paste-id.png)

- Compile the app with the generated **APP_ID**. Note: Call `cmake` to update the new APP_ID variable. Eventually, you should see the same APP_ID from the `app_merging.py` script output: `INFO:root:ID             : 03AD5A741E38A35F`

  ![Build output](assets/compiled-app.png)

- Create `config.json` file. Detailed instructions are [here](app-config-json.md) 

  ![Config](assets/config-json.png)

- Pack the resulting `*.uapp`, `icon.png`, and `config.json` into a `*.zip` archive.

  ![Archive](assets/archive.png)

- At the app page, click the **Version** tab.

  ![Version Tab](assets/app-version-tab.png)

- Click **Upload New**.

  ![Upload Dialog](assets/app-upload.png)

- Upload the resulting `*.zip` file.

  ![Upload](assets/app-version-upload.png)

- After upload successed, click **Release** to publish the app.

  ![Uploaded](assets/uploaded-app.png)

- Confirm by clicking **Confirm and Publish**

  ![Publish](assets/publish.png)

### Via PR to https://github.com/UNAWatch/una-apps for Open Source Apps

- Apply a PR to https://github.com/UNAWatch/una-apps.
  
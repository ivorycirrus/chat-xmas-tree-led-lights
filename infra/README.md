# Setup infra on AWS

## 1 Pre-requirements
* node.js : 20.x or higher

## 2. Quick Install
Project deployments are tested on Linux/MacOS environments.

### 2.1 Project Config
Edit config files in ```infra/config```
* config.json

### 2.2 AWS Account
Create IAM user os SSO user with IAM Identity Center.
* (option 1) If you already configured ```aws profile```
```
export AWS_PROFILE=<your-profile-name> 
```
* (option 2) With secret key and secret access key
```
export AWS_ACCESS_KEY_ID=<your-access-key>
export AWS_SECRET_ACCESS_KEY=<your-secret-access-key>
export AWS_DEFAULT_REGION=ap-northeast-2
```

### 2.3 Tool install
* Install AWS CDK with tools
```
npm install -g aws-cdk pnpm
```

### 2.4 Project dependency install
```
pnpm install
```

### 2.5 (optional) CDK Bootstrap
If you have not been deployed AWS CDK application in the AWS accout, you have to bootstrap CDK Toolkit.
```
cdk bootstrap
```

### 2.6 Deploy infrastructure
```
cdk deploy --all --require-approval never
```

## 3 Clean up
```
cdk destroy --all
```

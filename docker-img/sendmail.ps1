<#
.SYNOPSIS
    Отправка оформленного HTML-письма через корпоративный SMTP-сервер.

.DESCRIPTION
    Скрипт вызывается ботом (send_email() в src/support.cpp) так:
        powershell sendmail.ps1 <получатель> <заголовок> <ссылка>
    Параметры SMTP берутся из аргументов, а если они не заданы - из переменных окружения:
        CHANSERV_SMTP_SERVER    адрес SMTP-сервера (обязательно)
        CHANSERV_SMTP_PORT      порт, по умолчанию 587
        CHANSERV_SMTP_FROM      адрес отправителя (обязательно)
        CHANSERV_SMTP_USER      логин, если сервер требует авторизацию
        CHANSERV_SMTP_PASSWORD  пароль
        CHANSERV_SMTP_SSL       0 - без STARTTLS/SSL, по умолчанию 1
    Пароль в аргументах не передаётся, чтобы он не попадал в список процессов.

.EXAMPLE
    ./sendmail.ps1 oleg@example.ru "Веб-портал Jira" "https://jira.example.ru/browse/TEKLA-42"

.EXAMPLE
    ./sendmail.ps1 -To oleg@example.ru -Title "Отчёт" -Message "Отчёт за неделю готов" -Url "https://example.ru/report"
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string]$To,

    [Parameter(Position = 1)]
    [string]$Title = "Веб-портал Jira",

    [Parameter(Position = 2)]
    [string]$Url = "",

    # Текст письма. По умолчанию формируется из ключа задачи Jira в конце ссылки
    [string]$Message = "",

    [string]$Subject = "",

    [string]$SmtpServer = $env:CHANSERV_SMTP_SERVER,

    [int]$Port = $(if ($env:CHANSERV_SMTP_PORT) { [int]$env:CHANSERV_SMTP_PORT } else { 587 }),

    [string]$From = $env:CHANSERV_SMTP_FROM,

    [string]$User = $env:CHANSERV_SMTP_USER,

    [bool]$UseSsl = ($env:CHANSERV_SMTP_SSL -ne "0")
)

$ErrorActionPreference = "Stop"

function Fail([string]$text, [int]$code) {
    [Console]::Error.WriteLine($text)
    exit $code
}

if ([string]::IsNullOrWhiteSpace($SmtpServer)) {
    Fail "Не задан SMTP-сервер: параметр -SmtpServer или переменная CHANSERV_SMTP_SERVER" 2
}
if ([string]::IsNullOrWhiteSpace($From)) {
    Fail "Не задан отправитель: параметр -From или переменная CHANSERV_SMTP_FROM" 2
}

function Encode([string]$text) {
    return [System.Net.WebUtility]::HtmlEncode($text)
}

# Ключ задачи Jira - последний сегмент ссылки .../browse/TEKLA-42
$issueKey = ""
if ($Url -match "/browse/([^/?#]+)") {
    $issueKey = $Matches[1]
}

if ([string]::IsNullOrWhiteSpace($Message)) {
    $Message = if ($issueKey) { "На вас назначена задача $issueKey." } else { "Для вас есть новое уведомление." }
}
if ([string]::IsNullOrWhiteSpace($Subject)) {
    $Subject = if ($issueKey) { "$Title`: задача $issueKey" } else { $Title }
}

$button = ""
if ($Url) {
    $safeUrl = Encode $Url
    $button = @"
            <tr>
              <td style="padding: 8px 32px 24px 32px;">
                <a href="$safeUrl" style="display: inline-block; padding: 12px 24px; background: #0052cc; color: #ffffff; text-decoration: none; border-radius: 4px; font-weight: bold;">Открыть</a>
                <p style="margin: 16px 0 0 0; font-size: 12px; color: #6b778c;">Если кнопка не работает, ссылку необходимо скопировать в браузер:<br><a href="$safeUrl" style="color: #0052cc;">$safeUrl</a></p>
              </td>
            </tr>
"@
}

$body = @"
<!DOCTYPE html>
<html lang="ru">
<head><meta charset="utf-8"><title>$(Encode $Subject)</title></head>
<body style="margin: 0; padding: 0; background: #f4f5f7; font-family: Segoe UI, Arial, sans-serif;">
  <table role="presentation" width="100%" cellpadding="0" cellspacing="0" style="background: #f4f5f7; padding: 24px 0;">
    <tr>
      <td align="center">
        <table role="presentation" width="600" cellpadding="0" cellspacing="0" style="max-width: 600px; background: #ffffff; border-radius: 6px; overflow: hidden;">
          <tr>
            <td style="padding: 20px 32px; background: #172b4d; color: #ffffff; font-size: 18px; font-weight: bold;">$(Encode $Title)</td>
          </tr>
          <tr>
            <td style="padding: 24px 32px 8px 32px; font-size: 15px; line-height: 1.5; color: #172b4d;">$(Encode $Message)</td>
          </tr>
$button
          <tr>
            <td style="padding: 16px 32px; border-top: 1px solid #dfe1e6; font-size: 12px; color: #6b778c;">Письмо отправлено автоматически ботом Chanserv, отвечать на него не нужно.</td>
          </tr>
        </table>
      </td>
    </tr>
  </table>
</body>
</html>
"@

$mail = New-Object System.Net.Mail.MailMessage
$client = New-Object System.Net.Mail.SmtpClient($SmtpServer, $Port)
try {
    $mail.From = $From
    $mail.To.Add($To)
    $mail.Subject = $Subject
    $mail.SubjectEncoding = [System.Text.Encoding]::UTF8
    $mail.Body = $body
    $mail.BodyEncoding = [System.Text.Encoding]::UTF8
    $mail.IsBodyHtml = $true

    $client.EnableSsl = $UseSsl
    if ($User) {
        $client.Credentials = New-Object System.Net.NetworkCredential($User, $env:CHANSERV_SMTP_PASSWORD)
    }

    $client.Send($mail)
    Write-Output "Письмо '$Subject' отправлено на $To"
    exit 0
}
catch {
    Fail "Ошибка отправки письма на ${To}: $($_.Exception.Message)" 1
}
finally {
    $mail.Dispose()
    $client.Dispose()
}

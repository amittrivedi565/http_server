## API Gateway Features

A simplistic API Gateway implementation with the following capabilities:

### Request Routing
- Define **custom routes** for different backend services.  
- Redirect requests seamlessly based on path or method.  
- Support for versioned APIs (e.g., `/v1`, `/v2`).  

### Request Validation
- Validate incoming requests against predefined **schemas**.  
- Ensure required headers, query parameters, and body formats.  
- Reject malformed requests before hitting backend services.  

### Rate Limiting
- Apply **per-route** or **per-client** request limits.  
- Configurable thresholds (e.g., `100 requests/min`).  
- Prevent abuse and protect backend services from overload.  

### Extensibility
- Middleware support for **logging**, **authentication**, and **metrics**.  
- Easy to extend with custom plugins.  

### Technology
- `C++`

